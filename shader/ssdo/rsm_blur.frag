#version 330 core
out vec3 FragColor;

in vec2 TexCoords;

uniform sampler2D rsmInput;
uniform sampler2D depthTex;
uniform vec2 texelSize;

void main() {
    float centerDepth = texture(depthTex, TexCoords).r;
    if (centerDepth >= 0.9999) {
        FragColor = vec3(0.0);
        return;
    }

    vec3 accum = vec3(0.0);
    float weightSum = 0.0;

    for (int y = -2; y <= 2; ++y) {
        for (int x = -2; x <= 2; ++x) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec2 uv = TexCoords + offset;

            float sampleDepth = texture(depthTex, uv).r;
            float depthWeight = exp(-abs(sampleDepth - centerDepth) * 90.0);
            float spatialWeight = exp(-float(x * x + y * y) / 7.0);
            float w = depthWeight * spatialWeight;

            accum += texture(rsmInput, uv).rgb * w;
            weightSum += w;
        }
    }

    FragColor = accum / max(weightSum, 0.0001);
}
