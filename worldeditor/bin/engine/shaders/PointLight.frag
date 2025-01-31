#pragma version

#include "Common.vert"
#include "BRDF.frag"

layout(location = 0) uniform mat4 t_model;
layout(location = 1) uniform mat4 t_view;

layout(location = 32) uniform sampler2D normalsMap;
layout(location = 33) uniform sampler2D diffuseMap;
layout(location = 34) uniform sampler2D paramsMap;
layout(location = 35) uniform sampler2D depthMap;
layout(location = 36) uniform sampler2D shadowMap;

layout(location = 0) in vec4 _vertex;

layout(location = 0) out vec4 rgb;

int sampleCube(const vec3 v) {
    vec3 vAbs = abs(v);
    if(vAbs.z >= vAbs.x && vAbs.z >= vAbs.y) {
        return (v.z < 0.0) ? 5 : 4;
    } else if(vAbs.y >= vAbs.x) {
        return (v.y < 0.0) ? 3 : 2;
    }
    return (v.x < 0.0) ? 1 : 0;
}

void main (void) {
    vec2 proj   = (0.5 * ( _vertex.xyz / _vertex.w ) + 0.5).xy;

    vec4 slice0 = texture( normalsMap,  proj );
    vec4 slice2 = texture( paramsMap,   proj );

    vec3 n      = normalize( 2.0 * slice0.xyz - vec3( 1.0 ) );

    // Light model LIT
    if(slice0.w > 0.33) {
        float depth = texture( depthMap, proj ).x;
        vec4 world  = getWorld(camera.mvpi, proj, depth );

        vec3 dir = light.position.xyz - (world.xyz / world.w);
        vec3 normDir = normalize(dir);
        float dist  = length(dir);

        float fall  = getAttenuation( dist, light.params.y ) * light.params.x;

        vec4 slice1 = texture( diffuseMap, proj );
        float rough = max( 0.01, slice1.w );
        float spec  = slice2.w;
        float metal = slice2.z;

        vec3 albedo = slice1.xyz;
        vec3 v      = normalize( camera.position.xyz - (world.xyz / world.w) );
        vec3 h      = normalize( normDir + v );

        float ln    = dot( normDir, n );

        float shadow = 1.0;
        if(light.shadows == 1.0) {
            int index = sampleCube(-normDir);
            vec4 offset = light.tiles[index];
            vec4 proj   = light.matrix[index] * world;
            vec3 coord  = (proj.xyz / proj.w);
            shadow  = getShadow(shadowMap, (coord.xy * offset.zw) + offset.xy, coord.z - light.bias);
        }

        vec3 refl   = mix(vec3(spec), albedo, metal) * getCookTorrance( n, v, h, ln, rough );
        vec3 result = albedo * (1.0 - metal) + refl;
        float diff  = max(getLambert( ln, light.params.x ) * fall, 0.0) * shadow;

        rgb = vec4( light.color.xyz * result * diff, 1.0 );
    } else {
        rgb = vec4( vec3(0.0), 1.0 );
    }
}
