#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inFragmentPosition;
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

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightColor = pushConsts.lightColor;
    vec3 lightPosition = pushConsts.lightPosition;

    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(inNormal);
    vec3 lightDirection = normalize(lightPosition - inFragmentPosition);  

    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;

    float specularStrength = 0.5;
    vec3 viewDir = normalize(pushConsts.camera - inFragmentPosition);
    vec3 reflectDirection = reflect(-lightDirection, norm);  
    float spec = pow(max(dot(viewDir, reflectDirection), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  

    vec3 color = pushConsts.color.xyz;
    if (pushConsts.useTexture) {
        color = texture(texSampler, inTexCoordinate).xyz;
    }

    vec3 result = (ambient + diffuse + specular) * color;

    outColor = vec4(result, 1.);
}
