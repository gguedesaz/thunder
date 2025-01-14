#pragma version

layout(location = 32) uniform sampler2D   rgbMap;

layout(location = 33) uniform vec2 direction;
layout(location = 34) uniform vec2 size;
layout(location = 35) uniform int steps;
layout(location = 36) uniform float curve[32];

layout(location = 1) in vec2 _uv0;

layout(location = 0) out vec4 rgb;

void main (void) {
    vec4 sum = texture( rgbMap, _uv0 ) * curve[0];
    for(int i = 1; i < steps; i++) {
        vec2 offset = float(i) * size * direction;

        sum += texture( rgbMap, _uv0 - offset ) * curve[i];
        sum += texture( rgbMap, _uv0 + offset ) * curve[i];
    }
    rgb = sum;
}
