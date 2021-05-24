#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;
out vec2 position;
out vec2 topleft;
out vec2 bottomright;

// Application data
uniform mat3 transform;
uniform mat3 projection;

// Animation data
uniform int curFrame;
uniform vec2 spriteSize;
uniform vec2 cols_rows;
uniform vec2 curFrameCoords;
uniform vec2 flipVec;

void main()
{
    // transform Original coords
    texcoord = in_texcoord;
    if(flipVec.x < 0) texcoord.x = -texcoord.x;
    if(flipVec.y < 0) texcoord.y = -texcoord.y;

    vec2 unit_size = spriteSize/16.0; // one tile is 16 pixels

    // Note: position goes from -0.5 to 0.5.
    // for Y, down is negative and up is positive.
    float halfwidth = 0.5/cols_rows.x;
    float halfheight = 0.5/cols_rows.y;
    topleft = (projection * transform * vec3(-halfwidth, halfheight, 1.0)).xy;
    bottomright = (projection * transform * vec3(halfwidth, -halfheight, 1.0)).xy;

    // start at top left
    vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
    vec3 finalpos = projection * transform * vec3(0.0, 0.0, 0.0);

    // add such that current frame is in the middle

    // One square is (0.5/3, 0.5/2)
    if(flipVec.x > 0)
        finalpos.x += unit_size.x/2.0/3.0 * (cols_rows.x/2-unit_size.x/2.0);
    else
        finalpos.x -= unit_size.x/2.0/3.0 * (cols_rows.x/2-unit_size.x/2.0);
    if(flipVec.y > 0)
        finalpos.y -= unit_size.y/2.0/2.0 * (cols_rows.y/2-unit_size.y/2.0);
    else
        finalpos.y += unit_size.y/2.0/2.0 * (cols_rows.y/2-unit_size.y/2.0);

    pos.xy = pos.xy + finalpos.xy;

    // now get animation frame offset
    float xpos = curFrameCoords.x;
    float ypos = curFrameCoords.y;

    // zero-index the offset
    xpos -= 1.0;
    ypos -= 1.0;

    if(flipVec.x > 0)
        pos.x -= xpos * unit_size.x/2.0/3.0;
    else
        pos.x += xpos * unit_size.x/2.0/3.0;

    if(flipVec.y > 0)
        pos.y += ypos * unit_size.y/2.0/2.0;
    else
        pos.y -= ypos * unit_size.y/2.0/2.0;


    position = pos.xy;
    // set final position
    gl_Position = vec4(pos.xy, in_position.z, 1.0);
}