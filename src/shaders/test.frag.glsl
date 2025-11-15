#version 450

layout(location = 0) in vec3 vertNormal;
layout(location = 1) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// Shadow projection parameters
const float orthoSize = 100.0;
const float nearPlane = 0.1;
const float farPlane  = 500.0;

// Scene UBO
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec3 lightDir;
    vec3 lightColor;
    vec3 ambientLight; 
    vec3 groundLight;
} scene;

// Per-object UBO (if you want to use normals, not mandatory yet)
layout(set = 1, binding = 0) uniform ObjectUBO {
    mat4 model;
    vec3 objectColor;
} object;

layout(set = 2, binding = 0) uniform sampler2D shadowSampler;

mat4 orthographic(float left, float right, float bottom, float top, float zNear, float zFar)
{
    return mat4(
        vec4(2.0 / (right - left), 0.0, 0.0, 0.0),
        vec4(0.0, 2.0 / (top - bottom), 0.0, 0.0),
        vec4(0.0, 0.0, 1.0 / (zFar - zNear), 0.0),
        vec4(-(right + left) / (right - left),
             -(top + bottom) / (top - bottom),
             -zNear / (zFar - zNear),
             1.0)
    );
}

mat4 lookAtDir(vec3 dir)
{
    // Construct orthonormal basis for the light direction
    vec3 forward = normalize(dir);
    vec3 up = abs(forward.y) > 0.9 ? vec3(0, 0, 1) : vec3(0, 1, 0);
    vec3 right = normalize(cross(up, forward));
    up = cross(forward, right);

    // Rotation-only matrix → transpose = inverse
    return transpose(mat4(
        vec4(right, 0.0),
        vec4(up, 0.0),
        vec4(-forward, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    ));
}

float saturate(float x){
    return clamp(x, 0.0, 1.0);
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

vec3 computeGroundLight(){
    return saturate(dot(vertNormal, vec3(0.0f, -1.0f, 0.0f)))*scene.groundLight*0.1;
}
float computeSpecularLight(){
    vec3 viewDir = normalize(getCameraPos() - fragWorldPos);
    vec3 reflectDir = reflect(-scene.lightDir, vertNormal); // reflect light towards normal
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    return spec;
}

float computeShadow(){
    mat4 lightView = lookAtDir(scene.lightDir);
    mat4 lightProj = orthographic(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
    
    vec4 lightSpacePos = lightProj * lightView * vec4(fragWorldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz/lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0; 
    float closestDepth = texture(shadowSampler, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(vertNormal, scene.lightDir)), 0.001);
    return closestDepth < currentDepth ? 1.0 : 0.0;
}
void main() {
    float diff = max(dot(vertNormal, scene.lightDir), 0.0);

    vec3 color = ((scene.lightColor * (computeSpecularLight() + diff) + scene.ambientLight*0.1 + computeGroundLight())*object.objectColor)*(1-computeShadow());
    outColor = vec4(color, 1.0);
}
