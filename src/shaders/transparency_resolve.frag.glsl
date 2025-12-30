#version 450


layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(set = 4, binding = 0) uniform CustomObjectUBO { 
    vec4 tint;
} customProps;

layout(set = 5, binding = 0) uniform sampler2D transparencyRenderTexture;

void main(){
    //vec2 coord = gl_FragCoord.xy/textureSize(transparencyRenderTexture, 0);
    vec4 sampleColor = texture(transparencyRenderTexture, gl_FragCoord.xy/textureSize(transparencyRenderTexture, 0));
    vec3 rgb = sampleColor.a > 0.0 ? sampleColor.rgb / sampleColor.a : vec3(0.0);
    outColor = vec4(rgb, sampleColor.a);

 
    

}