#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;
layout(location = 5) in vec2 inUV;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outTangent;
layout(location = 2) out vec3 outBitangent;

layout(location = 3) out vec3 fragWorldPos;
layout(location = 4) out vec2 fragUV;
layout(location = 5) out vec4 fragCamPos;

// Scene UBO (set = 0)
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    mat4 lightView;
    mat4 lightProj;
    vec3 lightColor;
    vec3 lightDir;
    vec3 ambientLightColor;
} scene;

// Per-object UBO (set = 1, dynamic)
layout(set = 1, binding = 0) uniform ObjectUBO {
    mat4 model;
} object;


void main() {
    vec4 worldPos = object.model * vec4(inPos, 1.0);
    
   
    mat3 normalMatrix = transpose(inverse(mat3(object.model)));
    outNormal = normalize(normalMatrix * inNormal);
    outTangent = normalize(normalMatrix * inTangent);
    outBitangent = normalize(normalMatrix * inBitangent);
    fragWorldPos = worldPos.xyz;
    fragCamPos = scene.proj * scene.view * worldPos ;
    gl_Position = fragCamPos;
    //gl_Position = scene.lightProj * scene.lightView * worldPos;
    fragUV = inUV;

}
