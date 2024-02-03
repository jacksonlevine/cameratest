#ifndef SOUNDFXSYSTEM_H
#define SOUNDFXSYSTEM_H

#include <vector>
#include <string>
#include "ringbuffer.h"
#include <sndfile.h>
#include <iostream>

typedef int SoundEffect;

struct SoundEffectSeries {
    std::vector<SoundEffect> series;
    int front = 0;
};

class SoundFXSystem {
public:
    SoundEffect add(std::string path);
    void play(SoundEffect sound);
    void playNextInSeries(SoundEffectSeries& series);
    std::vector<RingBuffer*> outputBuffers;
private:
    inline static size_t bufferSize = 256;
    std::vector<std::vector<float>> masterBuffers;
};

#endif