#version 450

layout(location = 0) in vec2 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(inColor, inColor.x + inColor.y, 1.);
}
