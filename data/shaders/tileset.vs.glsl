#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;

// Application data
uniform mat3 transform;
uniform mat3 projection;

// Tileset data
uniform vec2 tilesetCoords;
uniform vec2 tilesetSize;

void main()
{
    // transform Original coords
    texcoord = vec2(in_texcoord.x/tilesetSize.x, in_texcoord.y/tilesetSize.y) + tilesetCoords;
    vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
    gl_Position = vec4(pos.xy, in_position.z, 1.0);
}