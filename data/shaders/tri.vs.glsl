#version 330

in vec3 in_position;
in vec3 in_color;

out vec3 vcolor;

uniform mat3 transform;
uniform mat3 projection;

void main()
{
    vcolor = in_color;
    vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
    gl_Position = vec4(pos.xy, in_position.z, 1.0);
}
