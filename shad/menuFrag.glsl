#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
in float elementID;
uniform sampler2D ourTexture;
uniform float mousedOverElement;
uniform float clickedOnElement;
uniform float time;

float gaussian(float x, float mean, float stdDev) {
    float a = 1.0 / (stdDev * sqrt(2.0 * 3.14159265));
    float b = exp(-pow(x - mean, 2.0) / (2.0 * pow(stdDev, 2.0)));
    return a * b;
}


void main() {
    FragColor = texture(ourTexture, TexCoord);
    if(FragColor.a < 0.1) {
        discard;
    }

    if(elementID <= -67.0f && elementID >= -75.0f) {
        float peak = 1.0 + (abs(elementID) - 67)/2.0;
        float radius = 0.45;
        FragColor.r += gaussian(time*3.0, peak, radius);
        FragColor.g -= gaussian(time*3.0, peak, radius);
        FragColor.b -= gaussian(time*3.0, peak, radius);
    }

    if(clickedOnElement == elementID) {
        FragColor = vec4(vec3(1.0, 1.0, 1.0) - FragColor.rgb, 1.0);
    } else if(mousedOverElement == elementID) {
        FragColor = FragColor + vec4(0.3, 0.3, 0.3, 0.0);
    }
    if(elementID == -99.0f) {
        FragColor = FragColor - vec4(0.5, 0.5, 0.5, 0.0);
    }
    if(elementID == -97.0f) {
        discard;
    }
    if(elementID == -98.0f) {
        FragColor = FragColor - vec4(0.3, 0.3, 0.3, 0.0);
    }
}