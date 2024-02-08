#ifndef OPALFRAMES_H
#define OPALFRAMES_H

#include <fstream>
#include <string>
#include <vector>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <stb_image.h>
#include <iostream>

class OpalFrames {
public:
    OpalFrames();
    std::vector<GLubyte*> frames;
    void loadOpalFrames();

};

#endif