#version 330

in vec3 in_position;
in vec2 in_texcoord;

out vec2 texcoord;
out vec2 interp_pos;

void main()
{
    interp_pos = in_position.xy + vec2(0.5, 0.5);
    interp_pos.y = 1-interp_pos.y;
    gl_Position = vec4(in_position.xy*2, 0, 1.0);
    texcoord = in_texcoord;
}
