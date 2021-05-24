#version 330

uniform sampler2D screen_texture;
uniform sampler2D renderedShadow;
uniform float time;
uniform float light_str;

in vec2 texcoord;
in vec2 interp_pos;

uniform int num_lights;
uniform vec3[20] light_pos;
uniform vec3[20] light_cols;
uniform mat3 transform;
uniform mat3 screen_transform;

layout(location = 0) out vec4 color;

vec4 shadow (vec4 in_color, vec4 mask_color)
{
    vec4 color = in_color * max(mask_color.r, 0.3);
    return color;
}

void main()
{
    vec2 coord = texcoord;

    vec4 mask_color = texture(renderedShadow, coord) * 10.0;
    mask_color = clamp(mask_color, 0.0, 1.0);
    vec4 in_color = texture(screen_texture, coord);

    if(num_lights > 0) {
        vec3 out_col = in_color.xyz * 0.1;
        vec3 light_color = shadow(in_color, mask_color).xyz;
        float strength = 0;
        for (int i = 0; i < num_lights; i++) {
            vec2 transform_pos = vec2(screen_transform * vec3(interp_pos, 1.0));
            float str = 1/(1 + distance(transform_pos,light_pos[i].xy)/200.0);
            strength += str;
//            out_col += light_cols[i] * light_pos[i].z / 2 * str * mask_color.xyz;
        }
        out_col = light_color * min(strength, 1.0);
        color = vec4(out_col, 1.0);
//        vec2 transform_pos = vec2(screen_transform * vec3(interp_pos, 1.0));
//        float str = 1/(1+distance(transform_pos,light_pos)/100.0)*light_str;
//        color = color * str;
    }
    else color = in_color;

}
