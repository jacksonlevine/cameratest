#include "opalframes.h"

void OpalFrames::loadOpalFrames() {
    frames.clear();

    int imageNum = 1;
    std::string initialPath = "assets/of/image";

    std::string extension = ".png";
    std::cout << "First check: " << initialPath + std::to_string(imageNum) + extension << "\n";

    while(std::filesystem::exists(initialPath + std::to_string(imageNum) + extension)) {
        
        frames.push_back(new GLubyte);

        int width, height, numchans;

        frames[frames.size() - 1] = stbi_load((initialPath + std::to_string(imageNum) + extension).c_str(), &width, &height, &numchans, 0);


        std::cout << "Loaded: " << initialPath + std::to_string(imageNum) + extension << "\n";

        imageNum++;
    }
}

OpalFrames::OpalFrames()
{
    loadOpalFrames();
}