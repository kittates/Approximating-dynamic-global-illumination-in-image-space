#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 lightVP;

out vec3 FragPosWorld;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPosWorld = worldPos.xyz;
    gl_Position = lightVP * worldPos;
}
