#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gDepth;
uniform sampler2D ssdoTex;
uniform sampler2D rsmGiTex;
uniform samplerCube shadowCube;

uniform vec3 lightPosWS;
uniform vec3 viewPosWS;
uniform vec3 lightColor;
uniform mat4 invView;
uniform float farPlane;

uniform float ambientStrength;
uniform float rsmIntensity;
uniform float ssdoDetailStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;
uniform int uEnableSSDODetail;

const vec3 gridSamplingDisk[20] = vec3[](
    vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
    vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
    vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
);

float calculateShadow(vec3 fragPosWS, vec3 normalWS) {
    vec3 lightDir = normalize(lightPosWS - fragPosWS);
    float bias = max(0.03 * (1.0 - dot(normalWS, lightDir)), 0.0040);

    // Push receiver along normal to reduce self-shadowing stripes on flat walls.
    vec3 samplePosWS = fragPosWS + normalWS * (2.0 * bias);
    vec3 fragToLight = samplePosWS - lightPosWS;
    float currentDepth = length(fragToLight);
    if (currentDepth >= farPlane) {
        return 0.0;
    }

    float viewDistance = length(viewPosWS - fragPosWS);
    float diskRadius = mix(0.004, 0.03, clamp(viewDistance / farPlane, 0.0, 1.0));

    float shadow = 0.0;
    for (int i = 0; i < 20; ++i) {
        float closestDepth = texture(shadowCube, fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= farPlane;
        if (currentDepth - bias > closestDepth) {
            shadow += 1.0;
        }
    }

    return shadow / 20.0;
}

void main() {
    float depth = texture(gDepth, TexCoords).r;
    if (depth >= 0.9999) {
        FragColor = vec4(0.03, 0.03, 0.03, 1.0);
        return;
    }

    vec3 fragPosVS = texture(gPosition, TexCoords).rgb;
    vec3 normalVS = normalize(texture(gNormal, TexCoords).rgb);
    vec3 albedo = texture(gAlbedo, TexCoords).rgb;

    vec3 fragPosWS = (invView * vec4(fragPosVS, 1.0)).xyz;
    vec3 normalWS = normalize(mat3(invView) * normalVS);

    vec4 ssdo = texture(ssdoTex, TexCoords);
    float ssdoEnable = (uEnableSSDODetail != 0) ? 1.0 : 0.0;
    vec3 ssdoDetailIndirect = max(ssdo.rgb, vec3(0.0)) * ssdoDetailStrength * ssdoEnable;
    float ssdoDetailShadow = clamp(ssdo.a * ssdoDetailStrength * ssdoEnable, 0.0, 1.0);

    vec3 rsmCoarseIndirect = max(texture(rsmGiTex, TexCoords).rgb, vec3(0.0)) * rsmIntensity;

    vec3 toLight = lightPosWS - fragPosWS;
    float dist = length(toLight);
    vec3 lightDir = toLight / max(dist, 0.0001);

    float attenuation = 1.0 / (1.0 + 1.4 * dist + 3.2 * dist * dist);

    float nDotL = max(dot(normalWS, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * nDotL * albedo * lightColor * attenuation;

    vec3 viewDir = normalize(viewPosWS - fragPosWS);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normalWS, halfDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor * attenuation;

    float coarseShadow = calculateShadow(fragPosWS, normalWS);
    float combinedShadow = coarseShadow;
    if (ssdoEnable > 0.5) {
        // Detail shadow should mainly reinforce coarse penumbra, not create bands on fully lit walls.
        float edgeFactor = clamp(4.0 * coarseShadow * (1.0 - coarseShadow), 0.0, 1.0);
        combinedShadow = clamp(coarseShadow + ssdoDetailShadow * edgeFactor * (1.0 - coarseShadow), 0.0, 1.0);
    }

    vec3 ambient = ambientStrength * albedo;
    vec3 direct = (1.0 - combinedShadow) * (diffuse + specular);

    vec3 indirect = min(rsmCoarseIndirect + ssdoDetailIndirect, vec3(0.8));
    vec3 color = ambient + direct + indirect;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}
