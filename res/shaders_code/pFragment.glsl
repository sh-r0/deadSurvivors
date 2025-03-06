#version 450 core

layout(location = 0) in vec2 vTexCoord;

layout(location = 0) out vec4 outCol;

layout(location = 0, binding = 0) uniform sampler2D screenTexture;

const vec2 screenSize = ivec2(400, 300);
const vec2 wSize = vec2(1200,900);

const float curvature_c = 5.0f;
const float vignetteWidth_c = 30.0f;

vec2 curvedScreen(vec2 _uv) {
    vec2 res = _uv * 2 - 1.0f;
    vec2 o = res.yx / curvature_c;
    res += (res * o * o);

    return (res *0.5f + 0.5f);
}

vec3 vignette(vec2 _uv) {
    vec3 res;
    vec2 vign = vec2(vignetteWidth_c) / screenSize;
    vign = smoothstep(vec2(0.0f), vign, 1-abs(2*_uv - 1.0f));
    
    return vec3(vign.x * vign.y);
}

vec3 crtLines(vec2 _uv) {
    vec3 res = vec3(0.0);
    int pCoord = int(_uv.y * wSize.y / 2);
    switch(pCoord % 3) {
        case 0: 
            res.r += 0.02f;
            break;
        case 1: 
            res.g += 0.02f;
            break;
        case 2: 
            res.b += 0.02f;
            break;
    };
    
    return res;
}

void main() {
    vec2 curvedCoord = curvedScreen(vTexCoord);
    outCol = texture(screenTexture, curvedCoord);    
    
    outCol.rgb *= 0.98f;
    outCol.rgb += crtLines(curvedCoord); 
    outCol.rgb *= vignette(curvedCoord);
    
    return;
}
