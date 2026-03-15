#version 330 core
layout (location = 0) out vec3 peelPosition;
layout (location = 1) out vec3 peelNormal;
layout (location = 2) out vec3 peelAlbedo;

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

uniform sampler2D uFrontDepth;
uniform vec2 uScreenSize;
uniform float uDepthEpsilon;

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
    vec2 uv = gl_FragCoord.xy / uScreenSize;
    float frontDepth = texture(uFrontDepth, uv).r;

    // Keep only fragments behind the first visible layer.
    if (frontDepth < 0.9999 && gl_FragCoord.z <= frontDepth + uDepthEpsilon) {
        discard;
    }

    peelPosition = fs_in.fragPosVS;
    peelNormal = normalize(fs_in.normalVS);
    peelAlbedo = resolveAlbedo();
}
