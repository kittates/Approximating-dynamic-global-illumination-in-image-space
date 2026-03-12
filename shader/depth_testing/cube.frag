#version 410 core
out vec4 FragColor;

// in vec2 TexCoords;
in vec3 Position;
in vec3 Normal;

// uniform sampler2D texture1;
uniform samplerCube skybox;
uniform vec3 cameraPos;
uniform float near;
uniform float far;

// float linearizeDepth(float depth);

void main()
{   
    vec3 camera_in = normalize(Position - cameraPos);
    // reflect
    // vec3 camera_out = reflect(camera_in, normalize(Normal));
    // refract 
    float ratio = 1.0 / 1.52;
    vec3 camera_out = refract(camera_in, normalize(Normal), ratio);
    // vec4 texColor = texture(skybox, camera_out);
    vec4 texColor = texture(skybox, vec3(camera_in.x, -camera_in.y, camera_in.z));
    // if(texColor.a < 0.1f) discard;  // terminate this fragment
    FragColor = vec4(texColor.rgb, 1.0);
    // FragColor = vec4(vec3(1.0f), 1.0f);
    // FragColor = vec4(vec3(), 1.0f);
    // float depth = (linearizeDepth(gl_FragCoord.z) - near) / (far - near);
    // FragColor = vec4(vec3(1.0f - depth), 1.0f);
    
}

// float linearizeDepth(float depth) {
//     float ndc = depth * 2.0 - 1.0;
//     return 2.0 * near * far / (near + far - ndc * (far - near));
// }