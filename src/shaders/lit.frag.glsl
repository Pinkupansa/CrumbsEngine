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
    vec2 atlasOffset;
    vec2 relativeTextureSize;
    vec2 normalmapAtlasOffset;
    vec2 normalmapRelativeTextureSize;
    vec2 tilingFactor;
    float specularity;
} customProps;
float saturate(float x) { return clamp(x, 0.0, 1.0); }


vec2 clampVec(vec2 a, vec2 minVal, vec2 maxVal) {
    return vec2(clamp(a.x, minVal.x, maxVal.x), clamp(a.y, minVal.y, maxVal.y));
}

vec4 sampleAtlasAA(vec2 uv, vec2 atlasOffset, vec2 relativeTexSixe, vec2 tilingFactor) {
    if(atlasOffset.r < 0.0) return vec4(1.0);
    vec2 pixelSize = vec2(
        length(dFdx(gl_FragCoord.xy)),
        length(dFdy(gl_FragCoord.xy))
    );

    vec2 dUVdx = dFdx(uv) * tilingFactor * relativeTexSixe * textureSize(textureAtlas[0], 0);
    vec2 dUVdy = dFdy(uv) * tilingFactor * relativeTexSixe * textureSize(textureAtlas[0], 0);

    float rho = max(dot(dUVdx, dUVdx), dot(dUVdy, dUVdy));
    float lod = 0.5 * log2(rho);
    lod = clamp(lod, 0.0, 7.0);

    int mip0 = int(floor(lod));

    float mip1 = mip0 + 1.0;

    const float H = 0.25;

    // stabilize the blend factor
    float t = smoothstep(H, 1.0 - H, lod - mip0);

    int index0 = clamp(int(mip0), 0, 7);
    int index1 = clamp(int(mip1), 0, 7);

    vec2 texel0 = 1.0 / vec2(textureSize(textureAtlas[index0], 0));
    vec2 texel1 = 1.0 / vec2(textureSize(textureAtlas[index1], 0)); 
    vec2 inset0 = 2*texel0;
    vec2 inset1 = 2*texel1;
    vec2 tiledUV = fract(uv * tilingFactor);
    vec2 atlasUV0 = atlasOffset
                + inset0
                + tiledUV * (relativeTexSixe - 2.0 * inset0);
    vec2 atlasUV1 = atlasOffset
                + inset1
                + tiledUV * (relativeTexSixe - 2.0 * inset1);
    vec4 color0 = texture(textureAtlas[index0], atlasUV0);
    vec4 color1 = texture(textureAtlas[index1], atlasUV1);

    vec4 finalColor = mix(color0, color1, t);


    return finalColor;
}

vec4 computeTexColor(vec2 uv, vec2 atlasOffset, vec2 relativeTexSixe, vec2 tilingFactor, float depth){
    return sampleAtlasAA(uv, atlasOffset, relativeTexSixe, tilingFactor);
}
vec3 computeNormal(){
    if(customProps.normalmapAtlasOffset.r < 0){
        return vertNormal;
    }
    vec3 normalMapSample = computeTexColor(fragUV, customProps.normalmapAtlasOffset, customProps.normalmapRelativeTextureSize, customProps.tilingFactor, fragCamPos.z).rgb;


    return normalize(normalMapSample.r*vertTangent + normalMapSample.g * vertBitangent + normalMapSample.b * vertNormal);
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
    return pow(max(dot(R, camDir), 0.0), 8.0) * customProps.specularity;
}
float computeDepth(vec4 lightSpacePos)
{   
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    float currentDepth = projCoords.z;

    //currentDepth = 2.0 * currentDepth - 1.0; // Remap to GL NDC for comparison
    //currentDepth = (2 * 30.0f*0.1f)/(30.0f + 0.1f - currentDepth * (30.0f - 0.1f));
    return currentDepth;
}
float sampleShadow(vec4 lightSpacePos)
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

    float bias = 0.0005;

    // In Vulkan: smaller depth = closer to light
    // If currentDepth > closestDepth (with bias) -> fragment is farther -> in shadow
    if (currentDepth - bias > closestDepth)
        return 0.2;  // Shadow
    else
        return 1.0;  // Lit
}

float computeShadow(vec4 lightSpacePos){
    // Perspective divide -> NDC
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    vec2 uv = projCoords.xy * 0.5 + 0.5;

    // If outside light frustum, consider fully lit
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        return 1.0;

    // PCF / blur parameters
    float shadow = 0.0;
    int kernelRadius = 1;         // 3x3 kernel
    float texelSize = 1.0 / 1024.0; // adjust to your shadow map resolution

    for(int x = -kernelRadius; x <= kernelRadius; ++x)
    {
        for(int y = -kernelRadius; y <= kernelRadius; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;

            // Build a temporary lightSpacePos offset by the UV offset
            // Note: sampleShadow only uses the UV inside for sampling
            vec4 offsetPos = lightSpacePos;
            offsetPos.xy += offset * 2.0; // map texel offset from [0,1] to NDC [-1,1]

            shadow += sampleShadow(offsetPos);
        }
    }

    shadow /= float((kernelRadius*2+1) * (kernelRadius*2+1));
    return shadow;
}

vec3 computeAmbient(vec3 N){
    return mix(scene.groundColor, scene.skyColor, N.y * 0.5 + 0.5)*0.01;
}
void main() {
    vec3 textureColor = computeTexColor(fragUV, customProps.atlasOffset, customProps.relativeTextureSize, customProps.tilingFactor, fragCamPos.z).rgb;

    
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
    float shadow = computeShadow(lightSpacePos);

    // Basic lighting calculation
    float diffuse = max(dot(N, L), 0.0);
    float specular = computeSpecularLight(N, L);

   
    
    vec3 lighting = (diffuse*textureColor + specular) * shadow * scene.lightColor + computeAmbient(N);
    //vec3 lighting = textureColor;
    //vec3 lighting = vec3(shadow, shadow, shadow);
    outColor = vec4(lighting, 1.0);

}
