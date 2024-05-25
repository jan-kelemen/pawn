#version 450

layout(location = 0) in vec4 inPosition;

layout(push_constant) uniform PushConsts {
    int transformIndex;
} pushConsts;

layout(binding = 0) uniform Transform {
    mat4 model;
    mat4 view;
    mat4 projection;
} transform;

layout(location = 0) out vec2 outColor;

void main() {
    gl_Position = transform.projection * transform.view * transform.model * vec4(inPosition.xyz, 1.0);
    outColor = vec2(inPosition.x, inPosition.x * 3);
}
 