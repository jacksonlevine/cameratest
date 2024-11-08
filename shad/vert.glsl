#version 330 core
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 texcoord;

out vec2 TexCoord;

uniform float yOffset;

void main()
{
    gl_Position = vec4(pos, 0.0, 1.0);
    gl_Position.y += yOffset;
    TexCoord = texcoord;
}