#version 450

layout(location = 0) in vec3 inPos;

// Scene UBO (set = 0)
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    vec3 lightPos;       // used as light direction (normalize on CPU)
    vec3 lightColor;
    vec3 ambientLight;
    vec3 groundLight;
} scene;

// Object UBO (set = 1)
layout(set = 1, binding = 0) uniform ObjectUBO {
    mat4 model;
    vec3 objectColor;
} object;

// Shadow projection parameters
const float orthoSize = 100.0;
const float nearPlane = 0.1;
const float farPlane  = 500.0;

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

void main()
{
    // Build light-space transform
    mat4 lightView = lookAtDir(scene.lightPos);
    mat4 lightProj = orthographic(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);

    vec4 worldPos = object.model * vec4(inPos, 1.0);
    gl_Position = lightProj * lightView * worldPos;
}
