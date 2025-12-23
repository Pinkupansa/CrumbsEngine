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

layout(set = 4, binding = 0) uniform CustomUBO{
    vec2 atlasOffset;
    vec2 relativeTextureSize;
    vec2 tilingFactor;
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

void main() {
    vec3 textureColor = computeTexColor(fragUV, customProps.atlasOffset, customProps.relativeTextureSize, customProps.tilingFactor, fragCamPos.z).rgb;
    outColor = vec4(textureColor, 1);
}
