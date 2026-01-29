#version 410 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);    //gl_Position接收vec4
    // vertexColor = vec4(aColor, 1.0f);
    // vertexColor = vec4(aColor, 1.0);
    // TexCoord = vec2(1.0 - aTexCoord.x, aTexCoord.y); // reverse vertically
    // TexCoord = aTexCoord;
}