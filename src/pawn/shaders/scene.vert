#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoordinate;

layout(push_constant) uniform PushConsts {
    vec4 color;
    vec3 camera;
    vec3 lightPosition;
    vec3 lightColor;
    int transformIndex;
    int useTexture;
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
layout(location = 3) out vec2 outTexCoordinate;

void main() {
    Transform transform = transformBuffer.transforms[pushConsts.transformIndex];
    gl_Position = transform.projection * transform.view * transform.model * vec4(inPosition.xyz, 1.0);
    outFragmentPosition = vec3(transform.model * vec4(inPosition.xyz, 1.0));
    outNormal = vec3(transform.model * vec4(inNormal.xyz, 1.0));

    outColor = pushConsts.color.xyz; 
    outTexCoordinate = inTexCoordinate;
}
 
