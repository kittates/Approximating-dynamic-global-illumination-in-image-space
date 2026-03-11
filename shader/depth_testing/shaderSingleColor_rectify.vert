#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float uOutline;

void main()
{
    // Normal = mat3(transpose(inverse(view * model))) * aNormal;  
    vec3 pos = aPos + uOutline * normalize(aNormal);
    gl_Position = projection * view * model * vec4(pos, 1.0f);
    
}
