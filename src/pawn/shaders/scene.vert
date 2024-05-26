#version 450

layout(location = 0) in vec4 inPosition;

layout(push_constant) uniform PushConsts {
    int transformIndex;
} pushConsts;

struct Transform {
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(std140, binding = 0) readonly buffer TransformBuffer {
    Transform transforms[];
} transformBuffer;

layout(location = 0) out vec2 outColor;

void main() {
    Transform transform = transformBuffer.transforms[pushConsts.transformIndex];
    gl_Position = transform.projection * transform.view * transform.model * vec4(inPosition.xyz, 1.0);
    outColor = vec2(0.1f + pushConsts.transformIndex / 40.f, 0.1f +  pushConsts.transformIndex / 40.f);
}
 