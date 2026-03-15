#version 330 core

in vec3 FragPosWorld;

uniform vec3 lightPos;
uniform float farPlane;

void main() {
    float lightDistance = length(FragPosWorld - lightPos);
    gl_FragDepth = lightDistance / farPlane;
}
