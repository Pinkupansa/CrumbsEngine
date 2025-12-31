#version 450


layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 4, binding = 0) uniform CustomObjectUBO { 
    vec4 tint;
} customProps;

layout(set = 5, binding = 0) uniform sampler2D screenRenderTexture;
float hash(float x) {
    x = fract(x * 0.1031);
    x *= x + 33.33;
    x *= x + x;
    return fract(x);
}

float rand(vec2 v) {
    return hash(v.x + v.y * 57.0);
}

void main(){
    //vec2 coord = gl_FragCoord.xy/textureSize(transparencyRenderTexture, 0);

    float uvNorm = pow(length(fragUV- vec2(0.5)), 0.8f);
    float maxNorm = pow(length(vec2(0.5)), 0.8f);
    vec4 sum = vec4(0.0);

    float weights = 0;
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            float w = rand(fragUV);
            weights += w;

            sum += texture(screenRenderTexture, fragUV + i*vec2(0.01f*uvNorm, 0) + j*vec2(0, 0.01f* uvNorm))* w;
            
        }
    }
    vec4 sampleColor = sum/weights * (maxNorm-uvNorm)/maxNorm;


    outColor = sampleColor;
}