#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;
out vec4 corners;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw * 1.5 + vec2(-0.25, -0.25);
    corners = vec4(0.0, 0.0, 1.0, 1.0); //vec4(0.25, 0.25, 1.25, 1.25);
}
