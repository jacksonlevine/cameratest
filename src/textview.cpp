#include "textview.h"

TextView::TextView() {

}

void TextView::create() {
    int width, height, nrChannels;
    unsigned char *data = stbi_load("assets/gui.png", &width, &height, &nrChannels, 0);
    if (!data)
    {
        std::cout << "TextView failed to load textTexture" << std::endl;
    }

    glGenTextures(1, &textTexture);
    glBindTexture(GL_TEXTURE_2D, textTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &vertShader, NULL);
    glCompileShader(vertexShader);
    glShaderSource(fragmentShader, 1, &fragShader, NULL);
    glCompileShader(fragmentShader);
    GLint success;
    char errorLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
        std::cerr << "TextView vert shade comp error:" << errorLog << '\n';
    }
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
        std::cerr << "TextView frag shade comp error:" << errorLog << '\n';
    }
    shader = glCreateProgram();
    glAttachShader(shader, vertexShader);
    glAttachShader(shader, fragmentShader);
    glLinkProgram(shader);
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
        std::cerr << "TextView shader program link error:" << errorLog << '\n';
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void TextView::setTextNode(std::string name, std::string text, glm::vec2 pos) {
    auto nodeIt = nodes.find(name);
    if(nodeIt == nodes.end()) {
        nodeIt = nodes.insert({name, TextNode{text, pos}}).first;
    }

    nodeIt->second.text = text;
    nodeIt->second.position = pos;
    updateDisplayData();
}

void TextView::removeTextNode(std::string name) {
    nodes.erase(name);
    updateDisplayData();
}

void TextView::updateDisplayData() {
    displayData.clear();

    float letHeight = (32.0f/windowHeight);
    float letWidth = (32.0f/windowWidth);
    
    for(auto& [name, node] : nodes) {
        const char* text = node.text.c_str();
        float lettersCount = std::strlen(text);

        glm::vec2 letterStart(node.position.x, -letHeight + node.position.y);

        GlyphFace glyph;

        for(int i = 0; i < lettersCount; i++) {
            glyph.setCharCode(static_cast<int>(text[i]));
            glm::vec2 thisLetterStart(letterStart.x + i*letWidth, letterStart.y);
            displayData.insert(displayData.end(), {
                thisLetterStart.x, thisLetterStart.y,                     glyph.bl.x, glyph.bl.y, -1.0f,
                thisLetterStart.x, thisLetterStart.y+letHeight,           glyph.tl.x, glyph.tl.y, -1.0f,
                thisLetterStart.x+letWidth, thisLetterStart.y+letHeight, glyph.tr.x, glyph.tr.y, -1.0f,

                thisLetterStart.x+letWidth, thisLetterStart.y+letHeight, glyph.tr.x, glyph.tr.y, -1.0f,
                thisLetterStart.x+letWidth, thisLetterStart.y,           glyph.br.x, glyph.br.y, -1.0f,
                thisLetterStart.x, thisLetterStart.y,                     glyph.bl.x, glyph.bl.y, -1.0f
            });
        }
    }
    uploaded = false;
}

void TextView::display() {
    glBindTexture(GL_TEXTURE_2D, textTexture);
    glUseProgram(shader);
    if(!uploaded) {
        glDeleteBuffers(1, &vbo);
        glGenBuffers(1, &vbo);
        bindGeometry(vbo, displayData.data(), displayData.size());
        uploaded = true;
    } else {
        bindGeometryNoUpload(vbo);
    }
    glDrawArrays(GL_TRIANGLES, 0, displayData.size()/5);
}