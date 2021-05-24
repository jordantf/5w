#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;
uniform float opacity;

void main() {
//    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    vec4 sampleText = texture(text, TexCoords);

    vec4 sampled;

    if(sampleText.g == 0.0 && sampleText.b == 0.0)
        sampled = vec4(1.0, 1.0, 1.0, sampleText.r);
    else
        sampled = vec4(sampleText.xyz, 1.0);

    color = vec4(textColor, opacity) * sampled;
}
