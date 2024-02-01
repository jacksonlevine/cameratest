#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#include <string>
#include <map>
#include <glm/glm.hpp>
#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <iostream>
#include <vector>
#include "glyphface.h"

struct TextNode {
    std::string text;
    glm::vec2 position;
};

struct TimedNode {
    std::string text;
    double timeStamp;
};

class TextView {
public:
    int windowHeight;
    int windowWidth;
    TextView();
    void create();

    void addMessageToHeap(
        std::string text
    );

    void setTextNode(
        std::string name,
        std::string text,
        glm::vec2 pos);
    void removeTextNode(
        std::string name);
    void display();

private:

    std::vector<TimedNode> heap;


    GLuint textTexture;
    GLuint vbo;
    std::vector<float> displayData;
    std::map<std::string, TextNode> nodes;

    inline static bool uploaded = false;

    void updateDisplayData();

    inline static const char* vertShader = R"glsl(
        #version 330 core
        layout (location = 0) in vec2 pos;
        layout (location = 1) in vec2 texcoord;
        layout (location = 2) in float elementid;

        out vec2 TexCoord;
        out float elementID;

        void main()
        {
            gl_Position = vec4(pos, 0.0, 1.0);
            TexCoord = texcoord;
            elementID = elementid;
        }
    )glsl";
    inline static const char* fragShader = R"glsl(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        in float elementID;
        uniform sampler2D ourTexture;
        void main() {
            FragColor = texture(ourTexture, TexCoord);
            if(FragColor.a < 0.1) {
                discard;
            }
        }
    )glsl";

    inline static GLuint shader;

    inline static void bindGeometry(GLuint vbo, const float *data, size_t dataSize) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, dataSize * sizeof(float), data, GL_STATIC_DRAW);

        GLint posAttrib = glGetAttribLocation(shader, "pos");
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

        GLint texAttrib = glGetAttribLocation(shader, "texcoord");
        glEnableVertexAttribArray(texAttrib);
        glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));

        GLint elementIdAttrib = glGetAttribLocation(shader, "elementid");
        glEnableVertexAttribArray(elementIdAttrib);
        glVertexAttribPointer(elementIdAttrib, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(4 * sizeof(float)));
    }

    inline static void bindGeometryNoUpload(GLuint vbo) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        GLint posAttrib = glGetAttribLocation(shader, "pos");
        glEnableVertexAttribArray(posAttrib);
        glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

        GLint texAttrib = glGetAttribLocation(shader, "texcoord");
        glEnableVertexAttribArray(texAttrib);
        glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));

        GLint elementIdAttrib = glGetAttribLocation(shader, "elementid");
        glEnableVertexAttribArray(elementIdAttrib);
        glVertexAttribPointer(elementIdAttrib, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(4 * sizeof(float)));
    }

};

#endif