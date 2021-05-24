#version 330

// Output color
layout(location = 0) out vec4 color;
in float f_opacity;
in vec2 texcoord;

uniform sampler2D sampler0;

void main()
{
    color = texture(sampler0, vec2(texcoord.x, texcoord.y));
    color.a = max(0.,f_opacity*color.a);
}
