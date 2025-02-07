#version 450 core

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 outCol;

layout(location = 0, binding = 0) uniform sampler2D screenTexture;

const ivec2 screenSize = ivec2(800, 600);

void main() {
    ivec2 pxCoord = ivec2(gl_FragCoord.xy * screenSize);
    outCol = texture(screenTexture, vTexCoord);
#if 0
    int colId = pxCoord.y % 3;
    switch(colId) {
        case 0:
            outCol.r += 0.02; 
            break;
        case 1:
            outCol.g += 0.02; 
            break;
        case 2:
            outCol.b += 0.02; 
            break;
    }
#endif
    return;
}
