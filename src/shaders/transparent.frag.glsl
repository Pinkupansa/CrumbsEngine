#version 450

layout(location = 0) in vec3 vertNormal;
layout(location = 1) in vec3 vertTangent;
layout(location = 2) in vec3 vertBitangent;
layout(location = 3) in vec3 fragWorldPos;
layout(location = 4) in vec2 fragUV;
layout(location = 5) in vec4 fragCamPos;
layout(location = 0) out vec4 outColor;

// Scene UBO
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    mat4 lightView;
    mat4 lightProj;
    vec3 lightColor;
    vec3 lightDir;
    vec3 groundColor;
    vec3 skyColor;
} scene;

// Object UBO
layout(set = 1, binding = 0) uniform ObjectUBO {
    mat4 model;

} object;

layout(set = 2, binding = 0) uniform sampler2D shadowSampler;

layout(set = 3, binding = 0) uniform sampler2D textureAtlas[8];

layout(set = 4, binding = 0) uniform CustomObjectUBO { 
    vec4 color;
} customProps;

