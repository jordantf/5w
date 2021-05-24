#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform vec3 highlight_col;
uniform vec4 background_col;
uniform vec3 tint_col;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
	vec4 bg_col = background_col;
	vec4 tex_color = texture(sampler0, vec2(texcoord.x, texcoord.y));
	if (tex_color.r > 0.9 && tex_color.g < 0.1 && tex_color.b < 0.1) {
		tex_color.xyz = tint_col;
	}
	bg_col = (1.0 - tex_color.w) * bg_col;
	color = vec4(fcolor, 1.0) * tex_color + vec4(highlight_col, 0.0) + bg_col;
}
