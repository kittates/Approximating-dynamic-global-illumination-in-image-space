#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture1;
uniform float near;
uniform float far;

float linearizeDepth(float depth);

void main()
{   
    // FragColor = texture(texture1, TexCoords);
    // FragColor = vec4(vec3(), 1.0f);
    float depth = (linearizeDepth(gl_FragCoord.z) - near) / (far - near);
    FragColor = vec4(vec3(1.0f - depth), 1.0f);
    
}

float linearizeDepth(float depth) {
    float ndc = depth * 2.0 - 1.0;
    return 2.0 * near * far / (near + far - ndc * (far - near));
}