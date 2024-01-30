#version 330 core
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 texcoord;
layout (location = 2) in float elementid;


out vec2 TexCoord;
out float elementID;

uniform float time;


float gaussian(float x, float mean, float stdDev) {
    float a = 1.0 / (stdDev * sqrt(2.0 * 3.14159265));
    float b = exp(-pow(x - mean, 2.0) / (2.0 * pow(stdDev, 2.0)));
    return a * b;
}

void main()
{
    gl_Position = vec4(pos, 0.0, 1.0);


    TexCoord = texcoord;
    elementID = elementid;


    if(elementID <= -67.0f && elementID >= -75.0f) {
        float peak = 1.0 + (abs(elementID) - 67)/2.0;
        float radius = 0.45;
        gl_Position.y -= gaussian(time, peak, radius);
    }
}