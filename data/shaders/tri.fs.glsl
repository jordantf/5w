#version 330

in vec3 vcolor;
uniform float lightStrength;

layout(location = 0) out vec4 color;

void main()
{
    color = vec4(1.0, 1.0, 1.0, lightStrength);
}