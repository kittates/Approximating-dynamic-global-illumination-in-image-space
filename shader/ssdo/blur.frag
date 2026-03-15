#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D ssdoInput;
uniform sampler2D depthTex;
uniform vec2 texelSize;

void main() {
    float centerDepth = texture(depthTex, TexCoords).r;
    if (centerDepth >= 0.9999) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 accum = vec4(0.0);
    float weightSum = 0.0;

    for (int y = -2; y <= 2; ++y) {
        for (int x = -2; x <= 2; ++x) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec2 sampleUV = TexCoords + offset;

            float sampleDepth = texture(depthTex, sampleUV).r;
            float depthWeight = exp(-abs(sampleDepth - centerDepth) * 80.0);
            float spatialWeight = exp(-float(x * x + y * y) / 6.0);
            float w = depthWeight * spatialWeight;

            accum += texture(ssdoInput, sampleUV) * w;
            weightSum += w;
        }
    }

    FragColor = accum / max(weightSum, 0.0001);
}
