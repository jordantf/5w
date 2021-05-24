#version 330 core
in vec2 TexCoords;
in vec4 corners;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;
uniform float outline_thickness;
uniform vec3 outline_color;
uniform float opacity;

void main() {
//    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    vec4 sampleText = texture(text, TexCoords);

    vec4 sampled;

    if(sampleText.g == 0.0 && sampleText.b == 0.0)
        sampled = vec4(1.0, 1.0, 1.0, sampleText.r);
    else
        sampled = vec4(sampleText.xyz, 1.0);


    float thickness = outline_thickness/10.0;
    bool outside = (-thickness > TexCoords.x) || (1.0 + thickness < TexCoords.x) ||
    (-thickness > TexCoords.y) || (1.0 + thickness < TexCoords.y);

    if (sampled.w < 1.0 || outside) {
        bool is_outline = false;
        for (int i = 0; i < 8; i++) {
            float x = (i % 4 == 0)? 0.0:
                (i > 4)? -1.0 : 1.0;
            float y = ((i + 2) % 4 == 0)? 0.0:
                ((i + 2) % 8 > 4)? -1.0 : 1.0;
            vec2 offset = vec2(x, y);
            if (texture(text, TexCoords + offset * thickness).r >= 0.5) {
                is_outline = true;
                break;
            }
        }
        if (is_outline) {
            color = vec4(outline_color, opacity);
            return;
        }
    }
    if (outside) {
        color = vec4(textColor, 0.0);
        return;
    }

    color = vec4(textColor, opacity) * sampled;
}
