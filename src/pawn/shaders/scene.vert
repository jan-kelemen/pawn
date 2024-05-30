#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;

layout(push_constant) uniform PushConsts {
    vec4 color;
    vec3 camera;
    vec3 lightPosition;
    vec3 lightColor;
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
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outFragmentPosition;

void main() {
    Transform transform = transformBuffer.transforms[pushConsts.transformIndex];
    gl_Position = transform.projection * transform.view * transform.model * vec4(inPosition.xyz, 1.0);
    outFragmentPosition = vec3(transform.model * vec4(inPosition.xyz, 1.0));

    outColor = vec3(pushConsts.color.xyz); 
    outNormal = inNormal;
}
 
