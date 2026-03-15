#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 fragPosVS;
    vec3 normalVS;
    vec2 texCoords;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 posVS = view * model * vec4(aPos, 1.0);
    vs_out.fragPosVS = posVS.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
    vs_out.normalVS = normalize(normalMatrix * aNormal);
    vs_out.texCoords = aTexCoords;

    gl_Position = projection * posVS;
}
