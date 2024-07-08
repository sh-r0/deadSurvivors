#version 450

layout(binding = 0) uniform UniformBuffer {
    vec2 uCameraPos;
    vec2 windowSize;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in uint inTexId;
layout(location = 3) in uint inIsGui;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out highp flat uint fragTexId; 

void main() {
    vec2 screen_space;
    if (inIsGui == 0) {
        screen_space = (vec2(inPosition.x, inPosition.y) - ubo.uCameraPos)/ubo.windowSize;
    }
    else {
        screen_space = (vec2(inPosition.x, inPosition.y)) / ubo.windowSize;
        screen_space = screen_space * 2 - 1;
    }
    gl_Position = vec4(screen_space, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragTexId = inTexId;
    
    return;
}
