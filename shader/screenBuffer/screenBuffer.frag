#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{   
    // common output
    FragColor  =vec4(texture(screenTexture, vec2(1.0 - TexCoords.x, TexCoords.y)).xyz, 1.0);

    // 反相
    // FragColor = vec4(1 - texture(screenTexture, TexCoords).xyz, 1.0);   

    // 灰度
    // vec4 color = texture(screenTexture, TexCoords);
    // float avg_color = 0.2126 * color.r + 0.7152 * color.g + 0.0722  * color.b;
    // FragColor = vec4(vec3(avg_color), 1.0);

    // kernal
    // float offset = 1.0 / 600.0;
    // vec2 offsets[9] = vec2[](
    //     vec2(-offset, offset), vec2(0.0f, offset), vec2(offset, offset),
    //     vec2(-offset, 0.0f), vec2(0.0f, 0.0f), vec2(offset, 0.0f), 
    //     vec2(-offset, -offset), vec2(0.0f,  -offset), vec2(offset, -offset)  
    // );
    // float kernal[9] = float[] (  // 锐化kernal
    //     -1, -1, -1,
    //     -1, 8, -1,
    //     -1, -1, -1
    // );
    // // float kernal[9] = float[] (      // blur
    // //     1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0,
    // //     2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0,
    // //     1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0
    // // );
    // // float kernal[9] = float[] ( // edge detact
    // //     -1.0, -1.0, -1.0,
    // //     -1.0, 8.0, -1.0,
    // //     -1.0, -1.0, -1.0
    // // );
    // vec3 res = vec3(0.0);
    // for(int i=0; i<9; i++) {
    //     res += kernal[i] * texture(screenTexture, TexCoords.st + offsets[i]).rgb;
    // }
    // FragColor = vec4(res, 1.0f);

}