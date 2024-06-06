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
    float outlineWidth;
    bool useTexture;
} pushConsts;

struct Transform {
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(std140, binding = 0) readonly buffer TransformBuffer {
    Transform transforms[];
} transformBuffer;

void main() {
    Transform transform = transformBuffer.transforms[pushConsts.transformIndex];

    // Extrude along normal
	vec4 pos = vec4(inPosition.xyz + inNormal * pushConsts.outlineWidth, 1.0f);
	gl_Position = transform.projection * transform.view * transform.model * pos;
}
 
