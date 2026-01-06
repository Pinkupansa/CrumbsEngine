#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 4, binding = 0) uniform CustomObjectUBO { 
    vec4 tint;
} customProps;

// Scene UBO
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    mat4 invProjView; // <-- precompute on CPU
    vec3 lightColor;
    vec3 lightDir;
    vec3 groundColor;
    vec3 skyColor;
} scene;

layout(set = 5, binding = 0) uniform sampler2D colors;
layout(set = 6, binding = 0) uniform sampler2DMS depths;

// Simple hash function (kept)
float hash33(vec3 p) {
    p = fract(p * vec3(0.1031, 0.11369, 0.13787));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

// ACES tone mapping
vec3 ACES(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x*(a*x+b)) / (x*(c*x+d)+e), 0.0, 1.0);
}

// Optimized ambient occlusion
float computeAmbientOcclusion() {
    // Pre-fetch fragment depth and texture size
    ivec2 coords = ivec2(gl_FragCoord.xy);
    float depth = texelFetch(depths, coords, 0).r;
    ivec2 texSize = textureSize(colors, 0);

    // Compute world-space position using precomputed inverse matrix
    vec2 screenCoords = 2.0 * fragUV - 1.0;
    vec4 pos = scene.invProjView * vec4(screenCoords, depth, 1.0);
    pos /= pos.w;

    // Gradients for expected depth changes
    float gradX = dFdx(depth * float(texSize.x));
    float gradY = dFdy(depth * float(texSize.y));

    float radius = 0.05;
    float occlusion = 0.0;

     vec3 offsets[8] = vec3[8](
        vec3(-1, -1, -1), vec3(1, -1, -1), vec3(-1, 1, -1), vec3(-1, -1, 1),
        vec3(1, 1, -1), vec3(-1, 1, 1), vec3(1, -1, 1), vec3(1, 1, 1)
    );

    // Precompute offsets
    for (int i = 0; i < 8; ++i) {
        
            vec3 offset = offsets[i];
            // Normalize by radius
            vec4 shift = vec4(offset * radius / length(offset), 0.0);
            vec4 newPos = pos + shift;

            // Project to clip space
            vec4 clip = scene.proj * scene.view * newPos;
            clip /= clip.w;
            vec2 newUV = clip.xy * 0.5 + 0.5;

            ivec2 newPixel = ivec2(newUV * texSize);
            float newDepth = texelFetch(depths, newPixel, 0).r;

            vec4 actualNewPos = scene.invProjView * vec4(newUV * 2.0 - 1.0, newDepth, 1.0);
            actualNewPos /= actualNewPos.w;

            vec3 diff = actualNewPos.xyz - pos.xyz;
            float expectedChange = (newUV.x - fragUV.x) * gradX + (newUV.y - fragUV.y) * gradY;

            if (expectedChange > newDepth - depth + 0.0003/(depth*depth) && length(diff) < 2.0 * radius) {
                occlusion += length(diff) * 20.0 * hash33(newPos.xyz); // simplified multiplier
            }
        }
    

    return clamp(1.0 - occlusion / 2.0, 0.0, 1.0);
}

void main() {
    float ao = computeAmbientOcclusion();
    outColor = texture(colors, fragUV) * ao;
}
