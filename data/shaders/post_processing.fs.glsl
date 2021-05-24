#version 330

uniform sampler2D screen_texture;
uniform sampler2D renderedScreen;
//uniform float time;
uniform float bloom_radius;
uniform float bloom_factor;

uniform float brightness;
uniform float saturation;
uniform float chromatic_aberration;

in vec2 texcoord;
in vec2 interp_pos;

uniform vec2 light_pos;
uniform mat3 transform;
uniform mat3 screen_transform;

layout(location = 0) out vec4 color;

const int bloomQuality = 6;
const int directions = 12;
const float PI_2 = 6.28318530718;//pi * 2

// Based off of https://github.com/CesiumGS/cesium/blob/master/Source/Shaders/Builtin/Functions/saturation.glsl
vec3 set_saturation(vec3 in_color) {
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(in_color, W));
    return mix(intensity, in_color, saturation * 2.0);
}

vec4 get_texture(sampler2D renderedScreen, vec2 coord) {
    if (chromatic_aberration > 0) {
        vec3 color;
        color.r = texture(renderedScreen, coord + vec2(chromatic_aberration/100.f, 0)).x;
        color.g = texture(renderedScreen, coord).y;
        color.b = texture(renderedScreen, coord + vec2(-chromatic_aberration/100.f,0)).z;
        return vec4(color, 1.0);
    }
    return texture(renderedScreen, coord);
}

void main()
{
    vec2 coord = texcoord;

    vec4 tex_color = get_texture(renderedScreen, coord);

    // BASED OFF OF https://xorshaders.weebly.com/tutorials/blur-shaders-5-part-2
    if (bloom_factor > 0.f) {
        vec4 blur_color = tex_color;
        vec2 radius = vec2(bloom_radius, bloom_radius)/10.f;
        for( float d=0.0; d < PI_2; d += PI_2/float(directions) )
        {
            for( float i=1.0/float(bloomQuality); i <= 1.0; i += 1.0/float(bloomQuality))
            {
                blur_color += get_texture(renderedScreen, coord + vec2(cos(d),sin(d)) * radius * i);
            }
        }
        blur_color /= float(bloomQuality)*float(directions)+1.0;
        tex_color += blur_color * bloom_factor;
    }

    tex_color = tex_color * (2.0 * brightness);
    color = vec4(clamp(set_saturation(tex_color.xyz), 0.0, 1.0), 1.0);
}
