#version 450

layout(binding = 1) uniform sampler2D fontTexture;
layout(binding = 2) uniform sampler2D textures[8];

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in highp flat uint fragTexId;

layout(location = 0) out vec4 outColor;

const vec3 fontColor = vec3(0.2, 0.0, 0.0);
const uint fontTexId = 1234;

void sampleText() {
    outColor = texture(fontTexture, fragTexCoord);
    if (outColor.r < 0.01) discard;
    float _alpha = clamp(0.5f + outColor.r, 0, 1);
    outColor = vec4(fontColor, _alpha);
    if (outColor.a < 0.01) discard;
    return;
}

void main() {
    if(fragTexId == fontTexId) {
        sampleText();
        return;
    }
    
    outColor = texture(textures[fragTexId], fragTexCoord);
    if (outColor.a < 0.1) discard;
        
    return;
}
