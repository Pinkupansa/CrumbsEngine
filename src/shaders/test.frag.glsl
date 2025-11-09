#version 450

layout(location = 0) in vec3 vertNormal;
layout(location = 1) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// Scene UBO
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec3 lightDir;
    vec3 lightColor;
} scene;

// Per-object UBO (if you want to use normals, not mandatory yet)
layout(set = 1, binding = 0) uniform ObjectUBO {
    mat4 model;
} object;
float saturate(float x){
    if(x < 0){
        return 0;
    }
    if(x > 1){
        return 1;
    }
    return x;
}
vec3 getCameraPos() {
    // Extract rotation (upper-left 3x3)
    mat3 rot = mat3(scene.view);

    // Extract translation (column 3)
    vec3 t = vec3(scene.view[3]); // column 3 = translation

    // Compute world-space camera position
    vec3 camPos = -transpose(rot) * t;
    return camPos;
}
float computeSpecularLight(){
    vec3 reflection = normalize(scene.lightDir - 2*dot(scene.lightDir, vertNormal))*vertNormal;
    vec3 camDir = normalize(getCameraPos() - fragWorldPos);
    return pow(saturate(dot(-camDir, reflection)), 8.0) * 500.0 ;
}
void main() {
    float diff = max(dot(vertNormal, scene.lightDir), 0.0);

    vec3 color = scene.lightColor * (computeSpecularLight() + diff);
    outColor = vec4(color, 1.0);
}
