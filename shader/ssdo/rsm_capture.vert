#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 fragPosWS;
    vec3 normalWS;
    vec2 texCoords;
} vs_out;

uniform mat4 model;
uniform mat4 lightVP;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    vs_out.fragPosWS = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vs_out.normalWS = normalize(normalMatrix * aNormal);
    vs_out.texCoords = aTexCoords;

    gl_Position = lightVP * worldPos;
}
