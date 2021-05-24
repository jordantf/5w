#version 330

// From vertex shader
in vec2 texcoord;
in vec2 position;
in vec2 topleft;
in vec2 bottomright;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;

// Output color
layout(location = 0) out vec4 color;

void main()
{
    if(position.y < topleft.y || position.y > bottomright.y) color = vec4(0);
    else if(position.x < topleft.x || position.x > bottomright.x) color = vec4(0);
    else color = vec4(fcolor, 1.0) * texture(sampler0, vec2(texcoord.x, texcoord.y));
}
