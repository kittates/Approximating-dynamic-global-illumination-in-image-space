#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gDepth;

uniform sampler2D peelPosition;
uniform sampler2D peelNormal;
uniform sampler2D peelAlbedo;
uniform sampler2D peelDepth;

uniform sampler2D texNoise;

uniform vec3 uKernel[32];
uniform mat4 projection;
uniform mat4 view;
uniform mat4 invView;
uniform vec2 noiseScale;

uniform vec3 lightPosWS;
uniform float radius;
uniform float bias;
uniform float shadowMapTexel;

bool isProbeBelowSurface(vec3 probePosVS, vec2 uv) {
    float frontDepth = texture(gDepth, uv).r;
    if (frontDepth < 0.9999) {
        vec3 frontPosVS = texture(gPosition, uv).rgb;
        // View-space depth ordering must be done on z along the camera ray.
        if (frontPosVS.z > probePosVS.z + bias) {
            return true;
        }
    }

    float peeledDepth = texture(peelDepth, uv).r;
    if (peeledDepth < 0.9999) {
        vec3 peeledPosVS = texture(peelPosition, uv).rgb;
        if (peeledPosVS.z > probePosVS.z + bias) {
            return true;
        }
    }

    return false;
}

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void accumulateLayer(
    vec3 layerPos,
    vec3 layerNormal,
    vec3 layerAlbedo,
    float layerDepth,
    vec3 fragPos,
    vec3 fragNormal,
    float weight,
    inout vec3 indirect,
    inout float weightSum
) {
    if (layerDepth >= 0.9999) {
        return;
    }

    vec3 toSample = layerPos - fragPos;
    float dist = length(toSample);
    if (dist < (bias + 0.02) || dist > radius) {
        return;
    }

    vec3 wi = toSample / max(dist, 1e-4);
    float nDotWi = max(dot(fragNormal, wi), 0.0);
    if (nDotWi <= 0.0) {
        return;
    }

    float sampleFacing = max(dot(normalize(layerNormal), -wi), 0.0);
    float falloff = 1.0 - dist / radius;
    float distWeight = 1.0 / (1.0 + 25.0 * dist * dist);

    float contrib = weight * nDotWi * sampleFacing * falloff * falloff * distWeight;
    indirect += layerAlbedo * contrib;
    weightSum += weight;
}

void main() {
    float centerDepth = texture(gDepth, TexCoords).r;
    if (centerDepth >= 0.9999) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 fragPosVS = texture(gPosition, TexCoords).rgb;
    vec3 fragNormalVS = normalize(texture(gNormal, TexCoords).rgb);
    vec3 centerAlbedo = texture(gAlbedo, TexCoords).rgb;

    vec3 randomVec = texture(texNoise, TexCoords * noiseScale).xyz;
    randomVec = normalize(randomVec * 2.0 - 1.0);

    vec3 tangent = normalize(randomVec - fragNormalVS * dot(randomVec, fragNormalVS));
    vec3 bitangent = normalize(cross(fragNormalVS, tangent));
    mat3 TBN = mat3(tangent, bitangent, fragNormalVS);

    vec3 indirect = vec3(0.0);
    float weightSum = 0.0;

    for (int i = 0; i < 32; ++i) {
        vec3 samplePosVS = fragPosVS + TBN * uKernel[i] * radius;
        vec4 projected = projection * vec4(samplePosVS, 1.0);
        if (projected.w <= 0.0001) {
            continue;
        }

        vec3 ndc = projected.xyz / projected.w;
        vec2 sampleUV = ndc.xy * 0.5 + 0.5;
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
            continue;
        }

        accumulateLayer(
            texture(gPosition, sampleUV).rgb,
            texture(gNormal, sampleUV).rgb,
            texture(gAlbedo, sampleUV).rgb,
            texture(gDepth, sampleUV).r,
            fragPosVS,
            fragNormalVS,
            1.0,
            indirect,
            weightSum
        );

        accumulateLayer(
            texture(peelPosition, sampleUV).rgb,
            texture(peelNormal, sampleUV).rgb,
            texture(peelAlbedo, sampleUV).rgb,
            texture(peelDepth, sampleUV).r,
            fragPosVS,
            fragNormalVS,
            0.45,
            indirect,
            weightSum
        );
    }

    vec3 fragPosWS = (invView * vec4(fragPosVS, 1.0)).xyz;
    vec3 normalWS = normalize(mat3(invView) * fragNormalVS);
    vec3 toLightWS = lightPosWS - fragPosWS;
    float lightDist = length(toLightWS);
    vec3 lightDirWS = toLightWS / max(lightDist, 0.0001);

    float nDotL = max(dot(normalWS, lightDirWS), 0.0);
    float tanAlpha = sqrt(max(1.0 - nDotL * nDotL, 0.0)) / max(nDotL, 0.15);
    float patchSize = 2.0 * lightDist * shadowMapTexel;
    float undefinedLength = clamp(0.5 * patchSize * tanAlpha, 0.0, 0.25);

    float shadowAccum = 0.0;
    float shadowNorm = 0.0;
    const int SHADOW_STEPS = 12;
    float jitter = hash12(TexCoords * vec2(389.1, 167.5));
    for (int i = 0; i < SHADOW_STEPS; ++i) {
        float t = (float(i) + jitter) / float(SHADOW_STEPS);
        vec3 probeWS = fragPosWS + lightDirWS * undefinedLength * t;
        vec3 probeVS = (view * vec4(probeWS, 1.0)).xyz;

        vec4 probeClip = projection * vec4(probeVS, 1.0);
        if (probeClip.w <= 0.0001) {
            continue;
        }

        vec3 probeNdc = probeClip.xyz / probeClip.w;
        vec2 probeUV = probeNdc.xy * 0.5 + 0.5;
        if (probeUV.x < 0.0 || probeUV.x > 1.0 || probeUV.y < 0.0 || probeUV.y > 1.0) {
            continue;
        }

        if (isProbeBelowSurface(probeVS, probeUV)) {
            shadowAccum += (1.0 - t);
        }
        shadowNorm += 1.0;
    }

    float detailShadow = clamp(shadowAccum / max(shadowNorm, 1.0), 0.0, 1.0);
    detailShadow = smoothstep(0.12, 0.62, detailShadow);

    float norm = max(weightSum, 1.0);
    vec3 detailIndirect = 0.2 * (indirect / norm) * centerAlbedo;
    detailIndirect = clamp(detailIndirect, vec3(0.0), vec3(0.12));

    FragColor = vec4(detailIndirect, detailShadow);
}
