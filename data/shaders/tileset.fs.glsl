#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform sampler2D normals;
uniform vec3 fcolor;
uniform vec3 highlight_col;
uniform vec3[20] light_pos;
uniform int num_lights;

// Output color
layout(location = 0) out vec4 color;

void main()
{
    vec4 tex = texture(sampler0, vec2(texcoord.x, texcoord.y));
    if (num_lights == 0) {
        color = tex;
        return;
    }
    vec3 normal = vec3(texture(normals, vec2(texcoord.x, texcoord.y)));

    // ambient
    vec3 ambient = tex.rgb * 0.0;
    vec3 diffuse = vec3(0,0,0);
    // diffuse
    for (int i = 0; i < num_lights; i++) {
        vec2 l_pos = light_pos[i].xy;
        vec3 ld = normalize(vec3(l_pos, 100.0));
        vec3 nd = normalize((normal-0.5)*2.);
        float dist = max((1 - length(l_pos)/1200), 0.1);
        float dot = max(0., ld.x * nd.x) + max(0., ld.y * -nd.y) + ld.z * nd.z;
        diffuse += tex.rgb * dot * light_pos[i].z * dist;
    }

    color = vec4(diffuse + ambient, tex.a) + vec4(highlight_col, 0.0);
}
