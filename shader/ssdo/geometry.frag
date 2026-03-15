#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;

in VS_OUT {
    vec3 fragPosVS;
    vec3 normalVS;
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
    gPosition = fs_in.fragPosVS;
    gNormal = normalize(fs_in.normalVS);
    gAlbedo = resolveAlbedo();
}
