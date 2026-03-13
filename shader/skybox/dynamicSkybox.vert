#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// out vec2 TexCoords;
out vec3 Position;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // TexCoords = aTexCoords;    
    Position = vec3(model * vec4(aPos, 1.0));
    //  aNormal是物体局部空间下的法线，需要进行修正
    Normal = transpose(inverse(mat3(model))) * aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
