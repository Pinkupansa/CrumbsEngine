#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 fragWorldPos;

// Scene UBO (set = 0)
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 lightColor;
    vec3 ambientLight; 
    vec3 groundLight;
} scene;

// Per-object UBO (set = 1, dynamic)
layout(set = 1, binding = 0) uniform ObjectUBO {
    mat4 model;
    vec3 objectColor;
} object;

void main() {
    vec4 worldPos = object.model * vec4(inPos, 1.0);
    gl_Position = scene.proj * scene.view * worldPos;
    mat3 normalMatrix = transpose(inverse(mat3(object.model)));
    outNormal = normalize(normalMatrix * inNormal);
    fragWorldPos = worldPos.xyz;
    
}
