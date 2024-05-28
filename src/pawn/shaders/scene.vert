#version 450

layout(location = 0) in vec4 inPosition;

layout(push_constant) uniform PushConsts {
    vec4 color;
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

layout(location = 0) out vec3 outColor;

void main() {
    Transform transform = transformBuffer.transforms[pushConsts.transformIndex];
    gl_Position = transform.projection * transform.view * transform.model * vec4(inPosition.xyz, 1.0);

    float colorOff = inPosition.y * 5;
    outColor = vec3(pushConsts.color.x + colorOff, pushConsts.color.y + colorOff, pushConsts.color.z + colorOff);
}
 
