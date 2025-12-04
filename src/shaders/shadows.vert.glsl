#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(set = 1, binding = 0) uniform ObjectUBO {
    mat4 model;
} object;

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    mat4 lightView;
    mat4 lightProj;
    vec3 lightColor;
    vec3 lightDir;
} scene;

void main() {
    vec4 worldPos = object.model * vec4(inPos, 1.0);
    gl_Position = scene.lightProj * scene.lightView * worldPos;
}

