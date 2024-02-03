#ifndef SOUNDFXSYSTEM_H
#define SOUNDFXSYSTEM_H

#include <vector>
#include <string>

typedef int SoundEffect;

class SoundFXSystem {
public:
    SoundEffect add(std::string path);
    void play(SoundEffect sound);
private:
    std::vector<std::vector<float>> masterBuffers;
    std::vector<std::vector<float>> outputBuffers;
};

#endif