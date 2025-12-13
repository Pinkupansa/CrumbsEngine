#version 450

layout(location = 0) in vec3 vertNormal;
layout(location = 1) in vec3 vertTangent;
layout(location = 2) in vec3 vertBitangent;
layout(location = 3) in vec3 fragWorldPos;
layout(location = 4) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

// Scene UBO
layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 view;
    mat4 proj;
    mat4 lightView;
    mat4 lightProj;
    vec3 lightColor;
    vec3 lightDir;
} scene;

// Object UBO
layout(set = 1, binding = 0) uniform ObjectUBO {
    mat4 model;
    vec2 atlasOffset;
    vec2 textureSize;
    vec2 normalmapAtlasOffset;
    vec2 normalmapTextureSize;
    vec2 tilingFactor;
} object;

layout(set = 2, binding = 0) uniform sampler2D shadowSampler;

layout(set = 3, binding = 0) uniform sampler2D textureAtlas;

float saturate(float x) { return clamp(x, 0.0, 1.0); }


vec2 clampVec(vec2 a, vec2 minVal, vec2 maxVal) {
    return vec2(clamp(a.x, minVal.x, maxVal.x), clamp(a.y, minVal.y, maxVal.y));
}
vec2 fractVec(vec2 v) {
    return v - floor(v);
}
vec4 sampleAtlas(vec2 uv, vec2 atlasOffset, vec2 textureSize, vec2 tilingFactor) {
    vec2 atlasUV =  clampVec(atlasOffset + fractVec(uv * tilingFactor) * textureSize, atlasOffset + 0.0002, atlasOffset + textureSize - 0.0002);
    return texture(textureAtlas, atlasUV);
}
vec3 computeNormal(){
    vec3 normalMapSample = sampleAtlas(fragUV, object.normalmapAtlasOffset, object.normalmapTextureSize, object.tilingFactor).rgb;
    vec3 normalMap = normalize(normalMapSample);

    return normalize(normalMap.r*vertTangent + normalMap.g * vertBitangent + normalMap.b * vertNormal);
}
vec3 getCameraPos() {
    mat3 rot = mat3(scene.view);
    vec3 t = vec3(scene.view[3]);
    return -transpose(rot) * t;
}

vec3 getLightDir()
{
    // Use the precomputed light direction from the Scene UBO (world space)
    return normalize(scene.lightDir);
}

float computeSpecularLight(vec3 N, vec3 L) {
    vec3 camDir = normalize(getCameraPos() - fragWorldPos);
    vec3 R = reflect(-L, N);
    return pow(max(dot(R, camDir), 0.0), 8.0) * 0.0;
}
float computeDepth(vec4 lightSpacePos)
{   
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    float currentDepth = projCoords.z;

    //currentDepth = 2.0 * currentDepth - 1.0; // Remap to GL NDC for comparison
    //currentDepth = (2 * 30.0f*0.1f)/(30.0f + 0.1f - currentDepth * (30.0f - 0.1f));
    return currentDepth;
}
float computeShadow(vec4 lightSpacePos, vec3 N, vec3 L)
{
    // Perspective divide -> NDC
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Convert NDC XY [-1,1] to UV [0,1]
    vec2 uv = projCoords.xy * 0.5 + 0.5;

    // If outside the light frustum, consider lit
    if (uv.x < 0.0 || uv.x > 1.0 ||
        uv.y < 0.0 || uv.y > 1.0)
        return 1.0;

    // Sample depth from shadow map (stored in [0,1] for Vulkan)
    float closestDepth = texture(shadowSampler, uv).r;

    // --- CRITICAL: If lightProj was built with GL conventions (NDC z in [-1,1]),
    // remap to Vulkan [0,1]. If already Vulkan (orthoRH_ZO), use projCoords.z directly.
    // Since mode 0 shows correct shadow map but mode 1 inverted, the projection is GL-style.
    float currentDepth = computeDepth(lightSpacePos);

    // Slope-scaled bias to reduce acne
    float nDotL = dot(N, L);
    float bias = max(0.005, 0.01 * (1.0 - nDotL));

    // In Vulkan: smaller depth = closer to light
    // If currentDepth > closestDepth (with bias) -> fragment is farther -> in shadow
    if (currentDepth - bias > closestDepth)
        return 0.2;  // Shadow
    else
        return 1.0;  // Lit
    
    
}

void main() {
    vec3 N = computeNormal(); 
    vec3 L = getLightDir();

    vec4 lightSpacePos = scene.lightProj * scene.lightView * vec4(fragWorldPos, 1.0);
    
    // DEBUG diagnostics
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    vec2 uv = projCoords.xy * 0.5 + 0.5;
    float shadowMapDepth = texture(shadowSampler, uv).r;
    //float shadowMapDepth = 0;

    float currentDepth = computeDepth(lightSpacePos);
    const int debugMode = 3;
    
   if (debugMode == 0) {
        // Show stored depth in shadow map
        outColor = vec4(vec3(shadowMapDepth), 1.0);
        return;
    }
    if (debugMode == 1) {
        // Show current fragment depth
        outColor = vec4(vec3(currentDepth), 1.0);
        return;
    }
    if (debugMode == 2) {
        // Show difference (positive = in shadow, negative = lit)
        float diff = currentDepth - shadowMapDepth;
        outColor = vec4(vec3(diff * 0.5 + 0.5), 1.0);
        return;
    }
    
    // Mode 3: Actual lighting
    // Calculate shadow factor using stored light direction
    float shadow = computeShadow(lightSpacePos, N, L);

    // Basic lighting calculation
    float diffuse = max(dot(N, L), 0.0);
    float specular = computeSpecularLight(N, L);

    vec3 textureColor = sampleAtlas(fragUV, object.atlasOffset, object.textureSize, object.tilingFactor).rgb;
    // Combine lighting with shadow (add small ambient)
    vec3 lighting = (diffuse*textureColor + specular) * shadow * scene.lightColor;
    //vec3 lighting = textureColor;
    //vec3 lighting = vec3(shadow, shadow, shadow);
    outColor = vec4(lighting, 1.0);
}
