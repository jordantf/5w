#version 330

// Input attributes
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 off_position;
layout(location = 2) in float opacity;

// Passed to fragment shader
out vec2 texcoord;
out float f_opacity;

// Application data
uniform mat3 transform;
uniform mat3 projection;

void main()
{
    texcoord = in_position.xy + vec2(0.5, 0.5);
    f_opacity = opacity;
    vec3 pos = projection * transform * vec3((in_position.xy)*(max(0., off_position.z)) + off_position.xy, 1.0); // why not simply *in_position.xyz ?
    gl_Position = vec4(pos.xy, in_position.z, 1.0);
}
