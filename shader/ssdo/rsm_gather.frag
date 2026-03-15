#version 330 core
out vec3 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDepth;
uniform sampler2D gAlbedo;
uniform sampler2D rsmFlux;
uniform sampler2D rsmNormal;
uniform sampler2D rsmDepth;

uniform mat4 invView;
uniform mat4 lightVP;
uniform mat4 invLightVP;
uniform float sampleRadius;
uniform int sampleCount;

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 reconstructRsmWorldPos(vec2 uv, float depth) {
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 world = invLightVP * clip;
    return world.xyz / max(world.w, 0.0001);
}

void main() {
    float depth = texture(gDepth, TexCoords).r;
    if (depth >= 0.9999) {
        FragColor = vec3(0.0);
        return;
    }

    vec3 fragPosVS = texture(gPosition, TexCoords).rgb;
    vec3 normalVS = normalize(texture(gNormal, TexCoords).rgb);
    vec3 receiverAlbedo = texture(gAlbedo, TexCoords).rgb;

    vec3 fragPosWS = (invView * vec4(fragPosVS, 1.0)).xyz;
    vec3 normalWS = normalize(mat3(invView) * normalVS);

    vec4 fragLS = lightVP * vec4(fragPosWS, 1.0);
    if (fragLS.w <= 0.0001) {
        FragColor = vec3(0.0);
        return;
    }

    vec3 fragNdcLS = fragLS.xyz / fragLS.w;
    vec2 baseUV = fragNdcLS.xy * 0.5 + 0.5;

    vec3 indirect = vec3(0.0);
    float valid = 0.0;

    int count = clamp(sampleCount, 1, 64);
    float minDist = max(0.05, 0.28 * sampleRadius);
    float minDist2 = minDist * minDist;

    for (int i = 0; i < count; ++i) {
        float fi = float(i);
        float angle = 6.2831853 * hash12(TexCoords * vec2(153.7, 91.3) + vec2(fi, fi * 0.37));
        float radius01 = sqrt(hash12(TexCoords * vec2(57.1, 211.4) + vec2(fi * 1.73, fi * 0.11)));
        vec2 offset = vec2(cos(angle), sin(angle)) * radius01 * sampleRadius;

        vec2 uv = baseUV + offset;
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
            continue;
        }

        float vplDepth = texture(rsmDepth, uv).r;
        if (vplDepth >= 0.9999) {
            continue;
        }

        vec3 vplPosWS = reconstructRsmWorldPos(uv, vplDepth);
        vec3 vplNormalWS = normalize(texture(rsmNormal, uv).rgb);
        vec3 vplFlux = texture(rsmFlux, uv).rgb;

        vec3 toVpl = vplPosWS - fragPosWS;
        float dist2 = dot(toVpl, toVpl);
        if (dist2 < minDist2) {
            continue;
        }
        vec3 wi = toVpl * inversesqrt(dist2 + 1e-6);

        float receiver = max(dot(normalWS, wi), 0.0);
        float sender = max(dot(vplNormalWS, -wi), 0.0);
        if (receiver <= 0.0 || sender <= 0.0) {
            continue;
        }

        float geom = receiver * sender;
        float attenuation = 1.0 / (dist2 + 0.04);

        // Lambertian receiving term (albedo / PI) and sample energy cap to avoid seam blow-up.
        vec3 sampleContribution = vplFlux * geom * attenuation;
        sampleContribution *= receiverAlbedo * 0.31830989;
        sampleContribution = min(sampleContribution, vec3(0.08));

        indirect += sampleContribution;
        valid += 1.0;
    }

    if (valid > 0.0) {
        float footprint = 3.1415926 * sampleRadius * sampleRadius;
        indirect *= footprint / valid;
    }

    FragColor = clamp(indirect, vec3(0.0), vec3(0.45));
}
