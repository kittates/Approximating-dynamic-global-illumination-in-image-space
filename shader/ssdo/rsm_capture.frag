#version 330 core
layout (location = 0) out vec3 rsmFlux;
layout (location = 1) out vec3 rsmNormal;

in VS_OUT {
    vec3 fragPosWS;
    vec3 normalWS;
    vec2 texCoords;
} fs_in;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;

    bool useTexture;
    vec3 ambientColor;
    vec3 diffuseColor;
    vec3 specularColor;
};

uniform Material material;
uniform vec3 uFallbackColor;
uniform vec3 lightPosWS;
uniform vec3 lightColor;

vec3 resolveAlbedo() {
    if (material.useTexture) {
        return texture(material.texture_diffuse1, fs_in.texCoords).rgb;
    }

    vec3 diffuse = material.diffuseColor;
    if (length(diffuse) < 0.001) {
        diffuse = uFallbackColor;
    }
    return diffuse;
}

void main() {
    vec3 normalWS = normalize(fs_in.normalWS);
    vec3 albedo = resolveAlbedo();

    vec3 toLight = lightPosWS - fs_in.fragPosWS;
    float dist = length(toLight);
    vec3 lightDir = toLight / max(dist, 0.0001);

    float attenuation = 1.0 / (1.0 + 1.4 * dist + 3.2 * dist * dist);
    float nDotL = max(dot(normalWS, lightDir), 0.0);

    rsmFlux = albedo * lightColor * nDotL * attenuation;
    rsmNormal = normalWS;
}
