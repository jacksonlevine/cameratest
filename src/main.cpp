#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector> 
#include <cstdlib>
#include <array>
#include <string>

#define TEXT_LOADER_IMPLEMENTATION
#include "textloader.h"


#include "textview.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "collcage.h"

#include <map>
#include <stack>

#include "crossmesh.h"
#include <optional>

#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <functional>
#include "guielement.h"

#include "portaudio.h"
#include <sndfile.h>
#include "inventory.h"
#include "soundfxsystem.h"


Inventory inventory;

bool drawingInv = false;
bool invDisplayDirty = true;

SoundFXSystem sfs;

SoundEffect doorSound = sfs.add("assets/sfx/door.mp3");

SoundEffect stoneStep1 = sfs.add("assets/sfx/stonestep1.mp3");
SoundEffect stoneStep2 = sfs.add("assets/sfx/stonestep2.mp3");
SoundEffect stoneStep3 = sfs.add("assets/sfx/stonestep3.mp3");
SoundEffect stoneStep4 = sfs.add("assets/sfx/stonestep4.mp3");

SoundEffect ladderSound = sfs.add("assets/sfx/ladder.mp3");

SoundEffect inventorySound = sfs.add("assets/sfx/inventory.mp3");
SoundEffect inventoryClose = sfs.add("assets/sfx/inventoryclose.mp3");

SoundEffect buttonHover = sfs.add("assets/sfx/buttonover.mp3");
SoundEffect buttonPress = sfs.add("assets/sfx/buttonpress.mp3");

SoundEffectSeries stoneStepSeries{
    {stoneStep1, stoneStep2, stoneStep3, stoneStep4}
};
float stepTimer = 0.0f;
float stepInterval = 0.3f;

int sampleRate = 0;
int channels = 0;


std::vector<float> loadAudioFile(const std::string& filename) {
    SF_INFO sfinfo;
    SNDFILE* sndfile = sf_open(filename.c_str(), SFM_READ, &sfinfo);
    if (sndfile == nullptr) {
        std::cerr << "Error opening audio file: " << filename << "\n";
        return {};
    }

    sampleRate = sfinfo.samplerate;
    channels = sfinfo.channels;

    std::vector<float> buffer(sfinfo.frames * sfinfo.channels);
    long long numFramesRead = sf_readf_float(sndfile, buffer.data(), sfinfo.frames);

    std::cout << "Num frames read: " << std::to_string(numFramesRead) << "\n";

    if (numFramesRead < sfinfo.frames) {
        std::cerr << "Error reading frames from audio file: " << filename << "\n";
    }

    sf_close(sndfile);
    return buffer;
}


PaStream* musicStream;
PaStream* sfxStream;

std::vector<float> audioData1; // First audio file data
std::vector<float> audioData2; // Second audio file data
std::vector<float> audioData3;

enum CurrentSong {
    MAINMENU,
    SHUFFLIN,
    SHOP
};

CurrentSong currentSong;

size_t dataIndex1 = 0;
size_t dataIndex2 = 0;
size_t dataIndex3 = 0;

static int musicCallback(const void* inputBuffer, void* outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData) {
    float* out = (float*)outputBuffer;


    for (size_t i = 0; i < framesPerBuffer; ++i) {
        if (currentSong == MAINMENU) {
            *out++ = audioData1[dataIndex1 * 2];     // Left channel
            *out++ = audioData1[dataIndex1 * 2 + 1]; // Right channel
            dataIndex1 = (dataIndex1 + 1) % (audioData1.size() / 2);
        } else if(currentSong == SHUFFLIN) {
            *out++ = audioData2[dataIndex2 * 2];     // Left channel
            *out++ = audioData2[dataIndex2 * 2 + 1]; // Right channel
            dataIndex2 = (dataIndex2 + 1) % (audioData2.size() / 2);
        } else if(currentSong == SHOP) {
            *out++ = audioData3[dataIndex3 * 2];     // Left channel
            *out++ = audioData3[dataIndex3 * 2 + 1]; // Right channel
            dataIndex3 = (dataIndex3 + 1) % (audioData3.size() / 2);
        }
    }
    return paContinue;
}

static int sfxCallback(const void* inputBuffer, void* outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData) {
    float* out = (float*)outputBuffer;
    std::fill(out, out + framesPerBuffer*2, 0.0f);
    for(RingBuffer * rbuf : sfs.outputBuffers) {
        if(rbuf->count > 0) {
            float input[512];
            rbuf->readOneBuffer(input);
            for (size_t i = 0; i < framesPerBuffer*2; i += 2) {
                out[i] = std::max(-1.0f, std::min(1.0f, input[i] + out[i]));
                out[i+1] = std::max(-1.0f, std::min(1.0f, input[i+1] + out[i+1]));
            }
        }

        
    }

    return paContinue;
}



GLuint VAO, VAO2, VBO;


std::function<void()>* loopFunc;

std::vector<GUIButton> *currentGuiButtons = nullptr;
float clickedOnElement = 0.0f;
float mousedOverElement = 0.0f;

float lastMousedOverElement = 0.0f;


bool JUMPKEYHELD = false;
bool JUMPING = false;


bool GOINGDOWN = false;
bool GOINGUP = false;

bool FUCKEDUP = false;

int FURTHESTLOADED = -1;


int LAYER = 0;


glm::vec2 cameraPosition = glm::vec2(0,0);
float cameraAngle = 0.0f;

int MAPWIDTH = 100;

GLubyte * MAP;
GLubyte * SHOPMAP;
bool INSHOP = false;

std::vector<GLubyte *> MAPS;

glm::vec2 viewedBlock;


int SWIDTH = 1280;
int SHEIGHT = 720;

int slotSelected = 0;


int forward = 0;
int backward = 0;
int left = 0;
int right = 0;


float FLOORLEVEL = 1.0f;

GLFWwindow * WINDOW;

GLuint SHADER_PROG1;

GLuint TEXTURE_ID;

GLuint MENUSHADER;
GLuint MENUTEXTURE;

GLuint LOGOTEXTURE;

GLuint SPLASHTEXTURE;

float FOVMULTIPLIER = 1.6f;


float MAXIMUMYSHEAR = 0.3f;

bool BUILDMODE = false;


float VIEWDISTANCE = 10.0f;

const int WIDTH = 320;
const int HEIGHT = 180;


float WALLHEIGHT = (float)HEIGHT/FOVMULTIPLIER;

const int NUMCHANNELS = 3;

float lastFrame = 0.0f;
float deltaTime = 0.0f;

float yOffset = 0.0f;

glm::ivec3 BACKGROUNDCOLOR = glm::ivec3(90, 0, 150);

GLubyte * PIXELS;

TextView textView;

std::function<void()> menuLoop;

std::function<void()> gameLoop;

std::function<void()> splashLoop;




void loadMap(int layer) {
    int width, height, nrChannels;
    if(layer > FURTHESTLOADED) {
        FURTHESTLOADED = layer;
        std::string mapPath("maps/map");
        mapPath += std::to_string(layer) + ".bmp";
        //std::cout << "Checking for: " << mapPath << "\n";

        std::filesystem::path dir("maps");
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directory(dir); // or create_directories(dir) if you might have nested directories
        }

        if(std::filesystem::exists(mapPath)) {
            //std::cout << "Found it!" << "\n";
            GLubyte * map = stbi_load(mapPath.c_str(), &width, &height, &nrChannels, 1);
            MAPS.push_back(map);
        } else {
            GLubyte * map = new GLubyte[100*100];
            for(int i = 0; i < 100; i++) {
                for(int k = 0; k < 100; k++) {
                    int ind = i * 100 + k;
                    if((float)rand()/RAND_MAX > 0.8f) {
                        map[ind] = 53;
                    } else
                    if((float)rand()/RAND_MAX > 0.99f) {
                        map[ind] = 54;
                    } else {
                        map[ind] = 0;
                    }
                }
            }
            MAPS.push_back(map);
        }
    }
}


void saveCameraPosition() {
    std::ofstream file("assets/camerasave", std::ios::trunc);
    file << cameraPosition.x << " " << cameraPosition.y << "\n";
    file << cameraAngle << "\n";
    file << LAYER << "\n";
}

void loadSavedCameraPosition() {
    if(std::filesystem::exists("assets/camerasave")) {
        std::ifstream file("assets/camerasave");
        std::string line;
        int lineIndex = 0;
        while(std::getline(file, line)) {
            std::istringstream linestream(line);
            std::string word;
            int localIndex = 0;
            while(linestream >> word) {
                if(lineIndex == 0) {
                    if(localIndex == 0) {
                        cameraPosition.x = std::stof(word);
                    }
                    if(localIndex == 1) {
                        cameraPosition.y = std::stof(word);
                    }
                 }
                 if(lineIndex == 1) {
                    cameraAngle = std::stof(word);
                 }
                 if(lineIndex == 2) {
                    LAYER = std::stoi(word);
                    for(int i = 0; i < LAYER; ++i) {
                        loadMap(i);
                    }
                 }
                localIndex++;
            }
            lineIndex++;
        }
        file.close();
    }
}



struct BlockType {
    GLubyte *texture;
    bool transparent;
    bool isCrossMesh;
};




void updateTime() {
    double currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
}






GLuint stoneTexture = 0;
int nrChannels;
int nrMapChannels;
GLubyte* stoneWallTexture;
GLubyte* floorTexture;
GLubyte* groundTexture;
GLubyte* plyWoodTexture;
GLubyte* glassTexture;
GLubyte* selectTexture;
GLubyte* plantTexture;

GLubyte* flowerTexture;

GLubyte* doorOpenTexture;

GLubyte* doorClosedTexture;
GLubyte* treeTexture;
GLubyte* rockTexture;
GLubyte* ladderTexture;
GLubyte* rubyTexture;
GLubyte* rubyGemTexture;
GLubyte* shopFloorTexture;
GLubyte* shopWallTexture;
GLubyte* inventoryTexture;

GLuint polyTexture;
GLuint polyTextureTransparent;

int invTexWidth, invTexHeight, invNumChans;

std::map<int, BlockType> blockTypes;
std::vector<std::pair<int, BlockType>> blockTypesOrdered;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if(yoffset > 0) {
        yoffset = 1;
    }
    if(yoffset < 0) {
        yoffset = -1;
    }
    slotSelected = (slotSelected + (int)yoffset) % 6;
    if(slotSelected < 0) {
        slotSelected = 5;
    }
    //std::cout << std::to_string(slotSelected) << "\n";
}


void loadTexture() {




    int width, height;
    stoneWallTexture = stbi_load("assets/stonewall.png", &width, &height, &nrChannels, 0);
    if (stoneWallTexture)
    {
        //std::cout << "channels: " << nrChannels << "\n";
    }
    else
    {
        std::cout << "Failed to load texture stonewall" << std::endl;
    }





    floorTexture = stbi_load("assets/floor.png", &width, &height, &nrChannels, 0);
    if (floorTexture)
    {
        //std::cout << "channels: " << nrChannels << "\n";
    }
    else
    {
        std::cout << "Failed to load texture floor" << std::endl;
    }

    plyWoodTexture = stbi_load("assets/plywood.png", &width, &height, &nrChannels, 0);
    if (plyWoodTexture)
    {
        //std::cout << "channels: " << nrChannels << "\n";
    }
    else
    {
        std::cout << "Failed to load texture plywood" << std::endl;
    }

    glassTexture = stbi_load("assets/glass.png", &width, &height, &nrChannels, 0);
    if (glassTexture)
    {
        //std::cout << "glass channels: " << nrChannels << "\n";
    }
    else
    {
        std::cout << "Failed to load texture glassTexture" << std::endl;
    }


    glGenTextures(1, &polyTexture);
    glBindTexture(GL_TEXTURE_2D, polyTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, plyWoodTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenTextures(1, &polyTextureTransparent);
    glBindTexture(GL_TEXTURE_2D, polyTextureTransparent);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, glassTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);




    selectTexture = stbi_load("assets/select.png", &width, &height, &nrChannels, 0);
    if (!selectTexture)
    {
        std::cout << "Failed to load texture selectTexture" << std::endl;
    }

    plantTexture = stbi_load("assets/plant.png", &width, &height, &nrChannels, 0);
    if (!plantTexture)
    {
        std::cout << "Failed to load texture plantTexture" << std::endl;
    }

    doorOpenTexture = stbi_load("assets/dooropen.png", &width, &height, &nrChannels, 0);
    if (!doorOpenTexture)
    {
        std::cout << "Failed to load texture doorOpenTexture" << std::endl;
    }

    doorClosedTexture = stbi_load("assets/doorclosed.png", &width, &height, &nrChannels, 0);
    if (!doorClosedTexture)
    {
        std::cout << "Failed to load texture doorClosedTexture" << std::endl;
    }

    flowerTexture = stbi_load("assets/flower.png", &width, &height, &nrChannels, 0);
    if (!flowerTexture)
    {
        std::cout << "Failed to load texture flowerTexture" << std::endl;
    }
    treeTexture = stbi_load("assets/tree.png", &width, &height, &nrChannels, 0);
    if (!treeTexture)
    {
        std::cout << "Failed to load texture treeTexture" << std::endl;
    }
    groundTexture = stbi_load("assets/ground.png", &width, &height, &nrChannels, 0);
    if (!groundTexture)
    {
        std::cout << "Failed to load texture groundTexture" << std::endl;
    }
    rockTexture = stbi_load("assets/rock.png", &width, &height, &nrChannels, 0);
    if (!rockTexture)
    {
        std::cout << "Failed to load texture rockTexture" << std::endl;
    }
    ladderTexture = stbi_load("assets/ladder.png", &width, &height, &nrChannels, 0);
    if (!ladderTexture)
    {
        std::cout << "Failed to load texture ladderTexture" << std::endl;
    }
    rubyTexture = stbi_load("assets/rubyore.png", &width, &height, &nrChannels, 0);
    if (!rubyTexture)
    {
        std::cout << "Failed to load texture rubyTexture" << std::endl;
    }
    rubyGemTexture = stbi_load("assets/ruby.png", &width, &height, &nrChannels, 0);
    if (!rubyGemTexture)
    {
        std::cout << "Failed to load texture rubyGemTexture" << std::endl;
    }

    SHOPMAP = stbi_load("assets/shopmap.bmp", &width, &height, &nrChannels, 1);
    if (!SHOPMAP)
    {
        std::cout << "Failed to load texture SHOPMAP" << std::endl;
    }

    shopFloorTexture = stbi_load("assets/shopfloor.png", &width, &height, &nrChannels, 0);
    if (!shopFloorTexture)
    {
        std::cout << "Failed to load texture shopFloorTexture" << std::endl;
    }


    shopWallTexture = stbi_load("assets/shopwall.png", &width, &height, &nrChannels, 0);
    if (!shopWallTexture)
    {
        std::cout << "Failed to load texture shopWallTexture" << std::endl;
    }

    inventoryTexture = stbi_load("assets/inventory.png", &invTexWidth, &invTexHeight, &invNumChans, 0);
    if (!inventoryTexture)
    {
        std::cout << "Failed to load texture ivenxtoryTuedxture" << std::endl;
    }


    blockTypes = {
    {255, BlockType{
        stoneWallTexture,
        false,
        false
    }},
    {100, BlockType{
        plyWoodTexture,
        false,
        false
    }},
    {150, BlockType{
        glassTexture,
        true,
        false
    }},
    {1, BlockType{
        selectTexture,
        true,
        false
    }},
    {50, BlockType{
        plantTexture,
        true,
        true
    }}
    ,
    {5, BlockType{
        doorClosedTexture,
        true,
        false
    }},
    {6, BlockType{
        doorOpenTexture,
        true,
        false
    }},
    {51, BlockType{
        flowerTexture,
        true,
        true
    }},
    {52, BlockType{
        ladderTexture,
        true,
        true
    }},
    {53, BlockType{
        floorTexture,
        false,
        false
    }},
    {54, BlockType{
        rubyTexture,
        false,
        false
    }},
    {55, BlockType{
        rubyGemTexture,
        true,
        true
    }},
    {56, BlockType{
        shopWallTexture,
        false,
        false
    }}
};
}

glm::ivec3 fuckedUpColorFromUV(float uvX, float uvY, GLubyte* texture, int nrChannels) {
    //assuming texture is always 32x32

    int x = std::round(uvX * 31.0f);
    int y = std::round((1.0f - uvY )* 31.0f);

    int index = y * 32 + x;

    int realIndex = index * nrChannels;
    
    if(nrChannels == 4) {
        if(texture[realIndex+3] == 0) {
            return glm::ivec3(-1,-1,-1);
        }
    }

    return glm::ivec3(texture[realIndex+3072+(int)(glfwGetTime()*60.0f)%10000], texture[realIndex+3072+1+(int)(glfwGetTime()*60.0f)%10000], texture[realIndex+3072+2+(int)(glfwGetTime()*60.0f)%10000]);
}

glm::ivec3 colorFromUV(float uvX, float uvY, GLubyte* texture, int nrChannels) {

    if(FUCKEDUP) {
        return fuckedUpColorFromUV(uvX, uvY, texture, nrChannels);
    }

    //assuming texture is always 32x32

    int x = std::round(uvX * 31.0f);
    int y = std::round((1.0f - uvY )* 31.0f);

    int index = y * 32 + x;

    int realIndex = index * nrChannels;
    
    if(nrChannels == 4) {
        if(texture[realIndex+3] == 0) {
            return glm::ivec3(-1,-1,-1);
        }
    }

    return glm::ivec3(texture[realIndex], texture[realIndex+1], texture[realIndex+2]);
}


glm::ivec4 colorWithAlphaFromUV(float uvX, float uvY, GLubyte* texture) {
    //assuming texture is always 32x32

    int x = std::round(uvX * 31.0f);
    int y = std::round((1.0f - uvY )* 31.0f);

    int index = y * 32 + x;

    int realIndex = index * 4;
    
    return glm::ivec4(texture[realIndex], texture[realIndex+1], texture[realIndex+2], texture[realIndex+3]);
}

glm::ivec4 colorWithAlphaFromUVImage(float uvX, float uvY, GLubyte* texture, int width, int height) {
    //assuming texture is always 32x32

    int x = std::round(uvX * (width -1));
    int y = std::round((1.0f - uvY )* (height -1));

    int index = y * width + x;

    int realIndex = index * 4;
    
    return glm::ivec4(texture[realIndex], texture[realIndex+1], texture[realIndex+2], texture[realIndex+3]);
}



void setPixel(int ind, GLubyte r,GLubyte g,GLubyte b) {
    PIXELS[ind*3] = r;
    PIXELS[ind*3 + 1] = g;
    PIXELS[ind*3 + 2] = b;
}

glm::ivec3 getPixelVal(int ind) {
    return glm::ivec3(PIXELS[ind*3], PIXELS[ind*3 + 1], PIXELS[ind*3 + 2]);
}

glm::vec2 directionFromAngle(float angle) {

    if(angle < 0) {
        angle = (2.0f * std::acos(-1.0f)) + angle;
    }
    float normalizedAngle = fmod(angle, 2.0f * std::acos(-1.0f));
    return glm::vec2(
        std::cos(normalizedAngle),
        std::sin(normalizedAngle) 
    );
}

int mapIndexFromCoord(int x, int z) {
    int zeroZero = MAPWIDTH/2 + (MAPWIDTH/2) * MAPWIDTH;

    int result = zeroZero + x + z * MAPWIDTH;

    if(result < MAPWIDTH*MAPWIDTH && result > -1) {
        return result;
    } else {
        return -1;
    }
}

int sampleMap(int x, int z) {
    int test = mapIndexFromCoord(x,z);
    if(test != -1) {
        if(!INSHOP) {
            if(blockTypes.find(MAPS[LAYER][test]) == blockTypes.end()) {
                        //std::cout << "cleaning this \n";
                        MAPS[LAYER][test] = 0;
            }
            return MAPS[LAYER][test];
        } else {
            if(blockTypes.find(SHOPMAP[test]) == blockTypes.end()) {
                        //std::cout << "cleaning this \n";
                        SHOPMAP[test] = 0;
            }
            return SHOPMAP[test];
        }
            
    } else {
        return -1;
    }
}

int pixelIndexFromCoord(int x, int y) {
    int result = y * WIDTH + x;

    if(result > -1 && result < WIDTH*HEIGHT) {
        return result;
    } else {
        return -1;
    }
}

float cameraY = 0.5f;

float speedMult = 3.75f;

LilCollisionCage collcage([](int x, int y){
    int samp = sampleMap(x,y);
    if(samp != 1 && samp != 0) {
        if(blockTypes.at(samp).isCrossMesh || samp == 6) {
            return false;
        }
        return true;
    }
    return false;
});

BoundingBox user(cameraPosition, glm::vec2(0,0));

struct HitPoint {
    int type;
    float travel;
    glm::vec2 testSpot;
    glm::vec2 hitSpot;
    glm::vec2 lastSpotBeforeHit;
    bool isCrossMesh = false;
    std::optional<float> distFromCornerPoint = std::nullopt;
};

void castRaysFromCamera() {

    collcage.update_readings(cameraPosition);

    glm::vec2 proposedPosition = cameraPosition;

    bool isWalking = false;

    if(!GOINGDOWN && !GOINGUP) {
        if(right) {
            proposedPosition += directionFromAngle(cameraAngle+((std::acos(-1.0f)/2.0f)))*speedMult*deltaTime;
            isWalking = true;
        }
        if(left) {
            proposedPosition += directionFromAngle(cameraAngle-((std::acos(-1.0f)/2.0f)))*speedMult*deltaTime;
            isWalking = true;
        }
        if(forward) {
            proposedPosition += directionFromAngle(cameraAngle)*speedMult*deltaTime;
            isWalking = true;
        }
        if(backward) {
            proposedPosition -= directionFromAngle(cameraAngle)*speedMult*deltaTime;
            isWalking = true;
        }
    }

    if(isWalking && !JUMPING) {
        if(stepTimer > stepInterval) {
            stepTimer = 0.0f;
            sfs.playNextInSeries(stoneStepSeries);
        } else {
            stepTimer += deltaTime;
        }
    }
    std::vector<glm::vec2> correctionsMade;

    user.set_center(proposedPosition, 0.2f);
    collcage.update_colliding(user);

    if(collcage.colliding.size() > 0) {
        for(Side side : collcage.colliding) {
            if(std::find(correctionsMade.begin(), correctionsMade.end(), LilCollisionCage::normals[side]) == correctionsMade.end()) {
                proposedPosition += LilCollisionCage::normals[side] * collcage.penetration[side];
                correctionsMade.push_back(LilCollisionCage::normals[side]);
            }
        }
    }

    cameraPosition = proposedPosition;

    glm::vec2 cameraDirection = directionFromAngle(cameraAngle);
    glm::vec2 cameraRight = directionFromAngle(cameraAngle+((std::acos(-1.0f)/2.0f)));

    for(int row = 0; row < HEIGHT; row++) {

        //now y means actual y
        float angle = (((((row - (HEIGHT / 2)) / (HEIGHT/2.0f) + 1.0f) * std::acos(-1.0f)) / 4.0f) - std::acos(-1.0f)/4.0f) ;

        angle *= FOVMULTIPLIER;

        glm::vec3 rayDir(cameraDirection.x, angle, cameraDirection.y);
        rayDir = glm::normalize(rayDir);

        float travel = 0.0f;

        glm::vec3 testSpot;

        bool hit = false;
        bool sky = false;

        bool top = false;
        
        static float RAYSTEP = 0.01f;

        for(float i = 0; i < VIEWDISTANCE; i+= RAYSTEP) {
            testSpot = glm::vec3(cameraPosition.x, cameraY + (FLOORLEVEL - 1.0f) * 0.75f, cameraPosition.y) + rayDir * i;

            if(testSpot.y >= 1.0f) {
                hit = true;
                break;
            }

            if(testSpot.y <= 0.0f) {
                if(LAYER == 0 && !INSHOP) {
                    sky = true;
                }
                top = true;
                hit = true;
                break;
            }
            travel += RAYSTEP;
        }

        if(hit) {
            if(sky) {

                for(int i = 0; i < WIDTH; i++) {

                    int ind = row * WIDTH + i;
                    
                    setPixel(ind, BACKGROUNDCOLOR.r, BACKGROUNDCOLOR.g, BACKGROUNDCOLOR.b);
                }

            }else {

                float FOV = std::acos(-1.0f)/2.0f /* your field of view in radians */;
                FOV *= FOVMULTIPLIER;



                float anglePerPixel = FOV / WIDTH;

                
                for(int i = 0; i < WIDTH; i++) {

                    int ind = row * WIDTH + i;

                    float angleOffset = (i - WIDTH / 2.0f) * anglePerPixel;

                    glm::vec2 directionForPixel = directionFromAngle(cameraAngle + angleOffset);

                    glm::vec3 worldPosition = glm::vec3(cameraPosition.x, cameraY, cameraPosition.y) + glm::vec3(directionForPixel.x, 0, directionForPixel.y) * travel * 1.5f;

                    float uvX = std::abs(std::fmod(worldPosition.x, 1.0f));
                    float uvY = std::abs(std::fmod(worldPosition.z, 1.0f));

                    glm::vec2 worldTile(std::round(worldPosition.x), std::round(worldPosition.z));
                    int tile = sampleMap(worldTile.x, worldTile.y);
                    
                    

                    glm::ivec3 color;
                    if(tile == 52) {
                        
                        if(top) {
                            if(LAYER > 0) {
                                loadMap(LAYER-1);
                                int tileAbove = MAPS[LAYER-1][mapIndexFromCoord(worldTile.x, worldTile.y)];
                                
                                if(tileAbove == 52 || tileAbove == 0) {
                                    color = glm::ivec3(0,0,0);
                                } else {
                                    color = colorFromUV(uvX, uvY, floorTexture, 3);
                                }
                            }
                            
                        }else {
                            loadMap(LAYER+1);
                            int tileBelow = MAPS[LAYER+1][mapIndexFromCoord(worldTile.x, worldTile.y)];
                            
                            if(tileBelow == 52 || tileBelow == 0) {
                                color = glm::ivec3(0,0,0);
                            } else {
                                color = colorFromUV(uvX, uvY, floorTexture, 3);
                            }
                        }
                        
                    } else if(INSHOP) {
                        color = colorFromUV(uvX, uvY, shopFloorTexture, 3);
                    } else {
                        color = colorFromUV(uvX, uvY, floorTexture, 3);
                    }

                    
                    
                    setPixel(ind, glm::mix(color.r, BACKGROUNDCOLOR.r, std::min(1.0f, travel*1.5f/VIEWDISTANCE)), glm::mix(color.g, BACKGROUNDCOLOR.g, std::min(1.0f, travel*1.5f/VIEWDISTANCE)), glm::mix(color.b, BACKGROUNDCOLOR.b, std::min(1.0f, travel*1.5f/VIEWDISTANCE)));
                }
            //std::cout << travel << "\n";
            }
        }

    }

    glm::vec2 viewRayDir = directionFromAngle(cameraAngle);
    glm::vec2 viewTestSpot = cameraPosition;
    for(float viewRay = 0.0f; viewRay < 3.1f; viewRay+=0.1f) {
        viewTestSpot += viewRayDir * 0.1f;
        glm::vec2 rounded(std::round(viewTestSpot.x), std::round(viewTestSpot.y));
        if(sampleMap(rounded.x, rounded.y) != 0 || viewRay >= 3.0f) {
            viewedBlock = rounded;
            break;
        }

    }

    for(int col = 0; col < WIDTH; col++) {

        std::stack<HitPoint> drawQueue;

        float angle = ((((col - (WIDTH / 2)) / (WIDTH/2.0f) + 1.0f) * std::acos(-1.0f)) / 4.0f) - std::acos(-1.0f)/4.0f;

        angle *= FOVMULTIPLIER;
        //std::cout << angle << "\n";
        glm::vec2 rayDir = glm::normalize(directionFromAngle(angle + cameraAngle));
        //std::cout << rayDir.x << " " << rayDir.y << "\n";

        float travel = 0;

        int hit = -1;

        glm::vec2 testSpot;

        glm::vec2 lastSpotBeforeHit;
        glm::vec2 hitSpot;

        static float RAYSTEP = 0.01f;
            testSpot = cameraPosition;
        for(float i = 0; i < VIEWDISTANCE + RAYSTEP; i += RAYSTEP) {
             testSpot += (rayDir * RAYSTEP);
             

            travel+=RAYSTEP;

            

            if(BUILDMODE) {

                if(glm::vec2(std::round(testSpot.x), std::round(testSpot.y)) == viewedBlock) {
                    drawQueue.push(HitPoint{
                        1,
                        travel,
                        testSpot,
                        glm::vec2(std::round(testSpot.x), std::round(testSpot.y)),
                        lastSpotBeforeHit
                    });
                    int sampled = sampleMap(std::round(testSpot.x), std::round(testSpot.y));
                    if(sampled == 0) {
                        while(glm::vec2(std::round(testSpot.x), std::round(testSpot.y)) == viewedBlock) {
                            //push past the select cube yo
                            lastSpotBeforeHit = glm::vec2(std::round(testSpot.x), std::round(testSpot.y));
                            testSpot += rayDir*0.01f;
                            travel += 0.01f;
                            i+=0.01f;
                        }
                    }
                    
                }
            }

            int sampled = sampleMap(std::round(testSpot.x), std::round(testSpot.y));

            hit = sampled;

            if(sampled != 0) {

                
                if(sampled != -1) {

                    

                    float rolledBack = 0.0f;
                    //std::cout << "travel was " << travel << "\n";
                    hitSpot = glm::vec2(std::round(testSpot.x), std::round(testSpot.y));

                    if(blockTypes.at(sampled).isCrossMesh) {
                        CrossMesh cmHere(hitSpot);
                        std::vector<Intersection> intersections = cmHere.getIntersections(rayDir, cameraPosition);
                        for(Intersection intersect : intersections) {
                            drawQueue.push(HitPoint{
                                sampled,
                                intersect.travel,
                                testSpot,
                                hitSpot,
                                lastSpotBeforeHit,
                                true,
                                intersect.distanceFromCornerPoint
                            });
                        }
                        //push past this crossmesh (don't keep adding it)
                        while (glm::vec2(std::round(testSpot.x), std::round(testSpot.y)) == hitSpot) {
                            lastSpotBeforeHit = glm::vec2(std::round(testSpot.x), std::round(testSpot.y));
                            testSpot += rayDir*0.01f;
                            travel += 0.01f;
                            i+=0.01f;
                        }
                        if(i >= VIEWDISTANCE) {
                            drawQueue.push(HitPoint{
                                    0,
                                    travel,
                                    testSpot,
                                    hitSpot,
                                    lastSpotBeforeHit
                                });
                            break;
                        }
                    } else {
                    
                        // while (sampleMap(std::round(testSpot.x), std::round(testSpot.y)) != 0 && sampleMap(std::round(testSpot.x), std::round(testSpot.y)) != -1) {
                        //     testSpot -= rayDir*0.001f;
                        //     travel -= 0.001f;
                        //     rolledBack += 0.001f;
                        // }

                        // testSpot += rayDir*0.005f;
                        //std::cout << "rolled back travel " << rolledBack << " to " << travel << "\n";
                        //std::cout << "pushing " << sampled << "\n";
                        drawQueue.push(HitPoint{
                            sampled,
                            travel,
                            testSpot,
                            hitSpot,
                            lastSpotBeforeHit
                        });

                        if(!(blockTypes.at(sampled).transparent)) {
                            break;
                        } else {
                            //push past this transparent (don't keep adding it)
                            bool cont = true;
                            while (cont) {
                                lastSpotBeforeHit = glm::vec2(std::round(testSpot.x), std::round(testSpot.y));
                                testSpot += rayDir*0.01f;
                                travel += 0.01f;
                                i+=0.01f;

                                int newSampled = sampleMap(std::round(testSpot.x), std::round(testSpot.y));
                                if(newSampled != -1 && newSampled != 0) {
                                    if(newSampled == 5 || newSampled == 6 || newSampled == sampled) {
                                        cont = true;
                                    } else {
                                        cont = false;
                                    }
                                } else {
                                    cont = false;
                                }
                            }
                            if(i >= VIEWDISTANCE) {
                                drawQueue.push(HitPoint{
                                        0,
                                        travel,
                                        testSpot,
                                        hitSpot,
                                        lastSpotBeforeHit
                                    });
                                break;
                            }
                        }
                    }


                } else {
                    //std::cout << "pushing " << sampled << "\n";
                    drawQueue.push(HitPoint{
                        sampled,
                        travel,
                        testSpot,
                        hitSpot,
                        lastSpotBeforeHit
                    });
                }
                
                
                
            } else {
                lastSpotBeforeHit = glm::vec2(std::round(testSpot.x), std::round(testSpot.y));
            }
        }

        if(hit == 0) {
            //std::cout << "pushing " << 0 << "\n";
            drawQueue.push(HitPoint{
                        0,
                        travel,
                        testSpot,
                        hitSpot,
                        lastSpotBeforeHit
                    });
        }


        int dqSizeWas = drawQueue.size();
        //std::cout << drawQueue.size() << "\n";
        while(!drawQueue.empty())
        {
            HitPoint h = drawQueue.top();

            drawQueue.pop();
            if(FLOORLEVEL < 1.66f && FLOORLEVEL > 0.36f) 
            {
            if(h.type == 0 || h.type == -1) {
                int height = ( WALLHEIGHT / h.travel) / 2.0f;

                int trav = 0;
                for(int i = HEIGHT/2  - (FLOORLEVEL - 1.0f) * (HEIGHT/(std::max(0.1f, h.travel)*2)); i < HEIGHT; i++) {
                    int ind = pixelIndexFromCoord(col, i);
                    if(ind != -1) {
                        if(trav < height) {
                            

                            setPixel(ind, BACKGROUNDCOLOR.r, BACKGROUNDCOLOR.g, BACKGROUNDCOLOR.b);
                        } else {
                            break;
                        }
                        
                    }
                    trav++;
                }
                trav = 0;
                for(int i = HEIGHT/2 - (FLOORLEVEL - 1.0f) * (HEIGHT/(std::max(0.1f, h.travel)*2)); i > -1; i--) {
                    int ind = pixelIndexFromCoord(col, i);
                    if(ind != -1) {
                        if(trav < height) {
                            setPixel(ind, BACKGROUNDCOLOR.r, BACKGROUNDCOLOR.g, BACKGROUNDCOLOR.b);
                        } else {
                            break;
                        }
                        
                    }
                    trav++;
                }
                
            } else {

                BlockType blockHere = blockTypes.at(h.type);



                float diffX = std::abs(h.hitSpot.x - h.lastSpotBeforeHit.x);
                float diffZ = std::abs(h.hitSpot.y - h.lastSpotBeforeHit.y); // Assuming Y is treated as Z

            

                float zmod = std::fmod(h.testSpot.y+0.5f, 1.0f);
                float xmod = std::fmod(h.testSpot.x+0.5f, 1.0f);

                float uvX;

                if(!h.isCrossMesh) {
                    if (diffX > diffZ) {
                        uvX = std::abs(zmod); 
                    } else {
                        uvX = std::abs(xmod); 
                    }
                } else {
                    uvX = h.distFromCornerPoint.value();
                }
                


                //std::cout << "hit on col " << col << " with travel " << travel << "\n";'

                int height = ( WALLHEIGHT / h.travel) / 2.0f;

                int trav = 0;
                for(int i = (HEIGHT/2)  - (FLOORLEVEL - 1.0f) * (HEIGHT/(std::max(0.1f, h.travel)*2)) + 1; i < HEIGHT; i++) {
                    int ind = pixelIndexFromCoord(col, i);
                    if(ind != -1) {
                        float uvY = 0.5f - (((float)trav/height) / 2.0f);
                        if(trav < height) {
                            

                            if(blockHere.transparent) {
                                glm::ivec4 color = colorWithAlphaFromUV(uvX, uvY, blockHere.texture);
                                if(color.a > 0) {
                                    if(color.a == 255 || dqSizeWas == 1) {
                                        setPixel(ind, glm::mix(color.r, BACKGROUNDCOLOR.r, std::min(1.0f, h.travel/VIEWDISTANCE)), glm::mix(color.g, BACKGROUNDCOLOR.g, std::min(1.0f, h.travel/VIEWDISTANCE)), glm::mix(color.b, BACKGROUNDCOLOR.b, std::min(1.0f, h.travel/VIEWDISTANCE)));
                                    } else {
                                        glm::ivec3 existingColor = getPixelVal(ind);
                                        existingColor.r = std::min(255, existingColor.r + (int)((float)glm::mix(color.r, BACKGROUNDCOLOR.r, h.travel/VIEWDISTANCE) * (color.a/255.0f)));
                                        existingColor.g = std::min(255, existingColor.g + (int)((float)glm::mix(color.g, BACKGROUNDCOLOR.g, h.travel/VIEWDISTANCE) * (color.a/255.0f)));
                                        existingColor.b = std::min(255, existingColor.b + (int)((float)glm::mix(color.b, BACKGROUNDCOLOR.b, h.travel/VIEWDISTANCE) * (color.a/255.0f)));
                                        setPixel(ind, existingColor.r, existingColor.g, existingColor.b);
                                    }

                                    
                                }
                            } else
                            {   
                                glm::ivec3 color = colorFromUV(uvX, uvY, blockHere.texture, 3);
                                setPixel(ind, glm::mix(color.r, BACKGROUNDCOLOR.r, h.travel/VIEWDISTANCE), glm::mix(color.g, BACKGROUNDCOLOR.g, h.travel/VIEWDISTANCE), glm::mix(color.b, BACKGROUNDCOLOR.b, h.travel/VIEWDISTANCE));
                            }
                            
                        } else {
                            break;
                        }
                        
                    }
                    trav++;
                }
                trav = 0;
                for(int i = HEIGHT/2  - (FLOORLEVEL - 1.0f) * (HEIGHT/(std::max(0.1f, h.travel)*2)); i > -1; i--) {
                    int ind = pixelIndexFromCoord(col, i);
                    if(ind != -1) {
                        float uvY = 0.5f + (((float)trav/height) / 2.0f);
                        if(trav < height) {
                            
                            
                            if(blockHere.transparent) {
                                glm::ivec4 color = colorWithAlphaFromUV(uvX, uvY, blockHere.texture);
                                if(color.a > 0) {
                                    if(color.a == 255 || dqSizeWas == 1) {
                                        setPixel(ind, glm::mix(color.r, BACKGROUNDCOLOR.r, std::min(1.0f, h.travel/VIEWDISTANCE)), glm::mix(color.g, BACKGROUNDCOLOR.g, std::min(1.0f, h.travel/VIEWDISTANCE)), glm::mix(color.b, BACKGROUNDCOLOR.b, std::min(1.0f, h.travel/VIEWDISTANCE)));
                                    } else {
                                        glm::ivec3 existingColor = getPixelVal(ind);
                                        existingColor.r = std::min(255, existingColor.r + (int)((float)glm::mix(color.r, BACKGROUNDCOLOR.r, h.travel/VIEWDISTANCE) * (color.a/255.0f)));
                                        existingColor.g = std::min(255, existingColor.g + (int)((float)glm::mix(color.g, BACKGROUNDCOLOR.g, h.travel/VIEWDISTANCE) * (color.a/255.0f)));
                                        existingColor.b = std::min(255, existingColor.b + (int)((float)glm::mix(color.b, BACKGROUNDCOLOR.b, h.travel/VIEWDISTANCE) * (color.a/255.0f)));
                                        setPixel(ind, existingColor.r, existingColor.g, existingColor.b);
                                    }
                                }
                            } else
                            {   
                                glm::ivec3 color = colorFromUV(uvX, uvY, blockHere.texture, 3);
                                setPixel(ind, glm::mix(color.r, BACKGROUNDCOLOR.r, h.travel/VIEWDISTANCE), glm::mix(color.g, BACKGROUNDCOLOR.g, h.travel/VIEWDISTANCE), glm::mix(color.b, BACKGROUNDCOLOR.b, h.travel/VIEWDISTANCE));
                            }
                                
                        } else {
                            break;
                        }
                        
                    }
                    trav++;
                }
            }
            }
        }
    }
}

bool mouseCaptured = false;
bool firstMouse = true;


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == 1) {
        clickedOnElement = mousedOverElement;
    } else {
        if(currentGuiButtons != nullptr) {
            for(auto &button : *currentGuiButtons) {
                if(button.elementID == clickedOnElement) {
                    button.myFunction();
                    sfs.play(buttonPress);
                }
            }
        }
        clickedOnElement = 0.0f;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if(mouseCaptured) {
            if(BUILDMODE) {
                int mapInd = mapIndexFromCoord(viewedBlock.x, viewedBlock.y);
                if(mapInd != -1) {
                    if(MAPS[LAYER][mapInd] != 0) {
                        if(inventory.addItem(ItemNode{MAPS[LAYER][mapInd], 1})) {
                            MAPS[LAYER][mapInd] = 0;
                        }
                    }
                    
                }
            }
            
        } else {
            if(loopFunc == &gameLoop && !drawingInv) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                mouseCaptured = true;
                mousedOverElement = 0.0f;
            }
            
        } 
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if(mouseCaptured) {
            if(BUILDMODE) {
                if(inventory.nodes[slotSelected].id != 0) {
                    int mapInd = mapIndexFromCoord(viewedBlock.x, viewedBlock.y);
                    if(mapInd != -1) {
                        if(MAPS[LAYER][mapInd] == 0) {
                            if(inventory.nodes[slotSelected].id == 52) {
                                loadMap(LAYER + 1);
                                if(MAPS[LAYER+1][mapInd] == 0 || MAPS[LAYER+1][mapInd] == 52) {
                                    MAPS[LAYER][mapInd] = 52;
                                } else {
                                    textView.addMessageToHeap("You cannot place a mineshaft here, there is no empty space below!");
                                }
                            } else {
                                MAPS[LAYER][mapInd] = inventory.nodes[slotSelected].id;
                            }
                            if(inventory.nodes[slotSelected].count == 1) {
                                inventory.nodes[slotSelected] = ItemNode{0,0};
                            }
                            inventory.nodes[slotSelected].count -= 1;
                        }
                    }
                }
            } else {
                int mapInd = mapIndexFromCoord(viewedBlock.x, viewedBlock.y);
               
                if(mapInd != -1) {


                    //door logic
                    if(MAPS[LAYER][mapInd] == 5) {
                        MAPS[LAYER][mapInd] = 6;
                        sfs.play(doorSound);
                    } else 
                    if(MAPS[LAYER][mapInd] == 6) {
                        MAPS[LAYER][mapInd] = 5;
                        sfs.play(doorSound);
                    }

                }
            }
            
            

        }
    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    static double lastX = 0;
    static double lastY = 0;

    if(mouseCaptured) {
        if(firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        double xDelta = xpos - lastX;
        double yDelta = ypos - lastY;

        lastX = xpos;
        lastY = ypos;

        xDelta *= 0.001;
        yDelta *= 0.001;

        cameraAngle += xDelta;

        yOffset = std::max(std::min(yOffset + yDelta, (double)MAXIMUMYSHEAR), -((double)MAXIMUMYSHEAR));
        
    }
}

void stepGoingDown() {
    static bool phaseTwo = false;
    static float goingDownTimer = 0.0f;
    static float gdTime = 0.5f;
    static bool incedLayer = false;
    static bool loadedMap = false;
    if(GOINGDOWN) {
        if(!loadedMap) {
            glm::ivec2 cameraTile(std::round(cameraPosition.x), std::round(cameraPosition.y));
            int mapInd = mapIndexFromCoord(cameraTile.x, cameraTile.y);

            loadMap(LAYER+1);

            MAPS[LAYER+1][mapInd] = 52;
            //std::cout << "Set 52 at mapind " << std::to_string(mapInd) << " on level " << std::to_string(LAYER+1) << "\n";
            loadedMap = true;
        }
        if(!phaseTwo) {
            if(goingDownTimer < gdTime) {
                goingDownTimer += deltaTime;
                FLOORLEVEL = 1.0f + (goingDownTimer / gdTime);
            } else {
                goingDownTimer = 0.0f;
                //GOINGDOWN = false;
                FLOORLEVEL = 0.0f;
                LAYER += 1;
                phaseTwo = true;
            }
        }
        if(phaseTwo) {
            if(goingDownTimer < gdTime) {
                goingDownTimer += deltaTime;
                FLOORLEVEL = 0.0f + (goingDownTimer / gdTime);
            } else {
                goingDownTimer = 0.0f;
                GOINGDOWN = false;
                loadedMap = false;
                FLOORLEVEL = 1.0f;
                phaseTwo = false;
            }
        }
        
    }
    if(GOINGUP) {
        if(!loadedMap) {
            glm::ivec2 cameraTile(std::round(cameraPosition.x), std::round(cameraPosition.y));
            int mapInd = mapIndexFromCoord(cameraTile.x, cameraTile.y);
            if(LAYER > 0) {
                loadMap(LAYER-1);
                MAPS[LAYER-1][mapInd] = 52;
            }
            
            loadedMap = true;
        }
        if(!phaseTwo) {
            if(goingDownTimer < gdTime) {
                goingDownTimer += deltaTime;
                FLOORLEVEL = 1.0f - (goingDownTimer / gdTime);
            } else {
                goingDownTimer = 0.0f;
                //GOINGDOWN = false;
                FLOORLEVEL = 2.0f;
                phaseTwo = true;
            }
        }
        if(phaseTwo) {
            if(goingDownTimer < gdTime) {
                goingDownTimer += deltaTime;
                FLOORLEVEL = 2.0f - (goingDownTimer / gdTime);
                if(goingDownTimer / gdTime > 0.3 && !incedLayer) {
                    LAYER -= 1;
                    incedLayer = true;
                }
            } else {
                goingDownTimer = 0.0f;
                GOINGUP = false;
                loadedMap = false;
                FLOORLEVEL = 1.0f;
                phaseTwo = false;
                incedLayer = false;
            }
        }
        
    }
}

void stepJumping() {
    static float maxJumpHeight = 0.45f;
    static float jumper = 0.0f;
    static bool jumpHeightReached = false;
    if(JUMPKEYHELD && !JUMPING) {
        JUMPING = true;
    }

    if(JUMPING) {
        if(jumper < maxJumpHeight && !jumpHeightReached) {
            jumper += ((maxJumpHeight+0.1)-jumper) * deltaTime * 9.0f;
        } else {
            jumpHeightReached = true;
        }

        if(jumpHeightReached) {
            if(jumper > 0.0f) {
                jumper -= ((maxJumpHeight+0.1)-jumper) * deltaTime * 9.0f;
            } else {
                sfs.playNextInSeries(stoneStepSeries);
                jumper = 0.0f;
                jumpHeightReached = false;
                JUMPING = false;
            }
        }

        FLOORLEVEL = std::min(1.0f, 1.0f - jumper);
    }
}

void startMainTheme() {
    if(currentSong != MAINMENU) {
        dataIndex1 = 0;
        currentSong = MAINMENU;
    }
}

void goToMainMenu() {
    glfwSetInputMode(WINDOW, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    firstMouse = true;
    mouseCaptured = false;
    loopFunc = &menuLoop;
    startMainTheme();
    static std::vector<GUIButton> buttons = {
        GUIButton(0.0f, 0.0f, "Singleplayer", 0.0f, 1.0f, [](){
            loopFunc = &gameLoop;
            currentSong = SHUFFLIN;
            currentGuiButtons = nullptr;
        }),
        GUIButton(0.0f, -0.1f, "Quit Game", 0.0f, 2.0f, [](){
            glfwSetWindowShouldClose(WINDOW, GLFW_TRUE);
        }),
    };
    for(GUIButton &button : buttons) {
        button.rebuildDisplayData();
    }
    currentGuiButtons = &buttons;
}

void goToEscapeMenu() {
    static std::vector<GUIButton> buttons = {
        GUIButton(0.0f, 0.2f, "Save and exit to main menu", 0.0f, 1.0f, [](){
            for(int i = 0; i <= FURTHESTLOADED; i++) {
                std::string mapPath("maps/map");
                mapPath += std::to_string(i) + ".bmp";
                int saveResult = stbi_write_bmp(mapPath.c_str(), MAPWIDTH, MAPWIDTH, 1, MAPS[i]);
            }
            if(!INSHOP) {
                saveCameraPosition();
            }
            goToMainMenu();
        }),
        GUIButton(0.0f, 0.1f, "Back to game", 0.0f, 2.0f, [](){
            currentGuiButtons = nullptr;
        }),
    };
    for(GUIButton &button : buttons) {
        button.rebuildDisplayData();
    }
    currentGuiButtons = &buttons;
}

void toggleShop() {
    INSHOP = !INSHOP;
    static glm::ivec3 outsideSkyColor;
    static CurrentSong previousSong;
    if(INSHOP) {
        previousSong = currentSong;
        currentSong = SHOP;
        outsideSkyColor = BACKGROUNDCOLOR;
        BACKGROUNDCOLOR = glm::ivec3(90,90,90);
        saveCameraPosition();
        cameraPosition = glm::vec2(0,0);
        BUILDMODE = false;
        textView.addMessageToHeap("Press K to leave the shop at any time.");
    } else {
        BACKGROUNDCOLOR = outsideSkyColor;
        currentSong = previousSong;
        loadSavedCameraPosition();
    }

}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_A)
    {
        left = action;
        
    }
    if (key == GLFW_KEY_D)
    {
        right = action;
    }

    if (key == GLFW_KEY_W) {
        forward = action;
    }
    if (key == GLFW_KEY_S) {
        backward = action;
    }
    if (key == GLFW_KEY_B && action == 1 && !INSHOP) {
        BUILDMODE = !BUILDMODE;
        std::string build("Build Mode ");
        build += BUILDMODE ? "ON" : "OFF";
        textView.setTextNode("Test", build, glm::vec2(0.4f,-0.4f));
    }
    if(key == GLFW_KEY_ESCAPE) {
        if(mouseCaptured && action == 1) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true;
            mouseCaptured = false;
            if(loopFunc == &gameLoop) {
                drawingInv = false;
                goToEscapeMenu();
            }
            
        } else 
        if(!mouseCaptured && action == 1 && !drawingInv){
            if(loopFunc == &gameLoop) {
                currentGuiButtons = nullptr;
            }

            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseCaptured = true;
            mousedOverElement = 0.0f;
        }
    }
    if((key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) && action == 1 && !GOINGUP) {
        glm::ivec2 cameraTile(std::round(cameraPosition.x), std::round(cameraPosition.y));
        int tile = sampleMap(cameraTile.x, cameraTile.y);
        if(tile == 52) {
            
            loadMap(LAYER+1);
            if(MAPS[LAYER+1][mapIndexFromCoord(cameraTile.x, cameraTile.y)] == 0 || MAPS[LAYER+1][mapIndexFromCoord(cameraTile.x, cameraTile.y)] == 52)
            {
                sfs.play(ladderSound);
                GOINGDOWN = true;
            }
                
        }
    }
    if(key == GLFW_KEY_SPACE) {
        glm::ivec2 cameraTile(std::round(cameraPosition.x), std::round(cameraPosition.y));
        int tile = sampleMap(cameraTile.x, cameraTile.y);
        if(tile == 52 && LAYER > 0) {
            if(action == 1 && !GOINGDOWN) {
            
                
                loadMap(LAYER-1);
                if(MAPS[LAYER-1][mapIndexFromCoord(cameraTile.x, cameraTile.y)] == 0 || MAPS[LAYER-1][mapIndexFromCoord(cameraTile.x, cameraTile.y)] == 52) {
                    sfs.play(ladderSound);
                    GOINGUP = true;
                }
                
            }
        } else {
            JUMPKEYHELD = action ? true : false;
        }
        
    }
    
    if(key == GLFW_KEY_EQUAL && action == 1) {
        FUCKEDUP = !FUCKEDUP;
    }


    if(key == GLFW_KEY_K && action == 1) {
        if(sampleMap(viewedBlock.x, viewedBlock.y) == 55 || INSHOP)
            toggleShop();
    }

    if(key == GLFW_KEY_E && action == 1 && loopFunc == &gameLoop) {
        drawingInv = !drawingInv;
        if(drawingInv) {
            sfs.play(inventorySound);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true;
            mouseCaptured = false;
        } else {
            sfs.play(inventoryClose);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseCaptured = true;
            mousedOverElement = 0.0f;
        }
    }

    // if(key == GLFW_KEY_P) {
    //     // FOVMULTIPLIER += 0.01f;
    //     // WALLHEIGHT = (float)HEIGHT/FOVMULTIPLIER;
    //     // std::cout << std::to_string(FOVMULTIPLIER) << "\n";
    //     FLOORLEVEL += 0.01f;
    // }
    // if(key == GLFW_KEY_O) {
    //     // FOVMULTIPLIER -= 0.01f;
    //     // WALLHEIGHT = (float)HEIGHT/FOVMULTIPLIER;
    //     // std::cout << std::to_string(FOVMULTIPLIER) << "\n";
    //     FLOORLEVEL -= 0.01f;
    // }
        
}


void drawSelectedBlock() {
    int squareWidth = 25;
    int startX = (int)((float)WIDTH * 0.75f);
    int startY = (int)(((float)HEIGHT + yOffset * 100) * 0.75f);
    if(inventory.nodes[slotSelected].id != 0) {
        for(int y = 0; y < squareWidth; y++) {
            for(int x = 0; x < squareWidth; x++) {
                int index = (startY + y) * WIDTH + (startX + x);
                float uvX = x / (float)squareWidth;
                float uvY = 1.0f - y / (float)squareWidth;

                if(blockTypes.at(inventory.nodes[slotSelected].id).transparent) {
                    glm::vec4 color = colorWithAlphaFromUV(uvX, uvY, blockTypes.at(inventory.nodes[slotSelected].id).texture);
                    glm::vec3 existingColor = getPixelVal(index);

                    existingColor.r = std::min(255, (int)(existingColor.r + color.r * (color.a / 255.0f)));
                    existingColor.g = std::min(255, (int)(existingColor.g + color.g * (color.a / 255.0f)));
                    existingColor.b = std::min(255, (int)(existingColor.b + color.b * (color.a / 255.0f)));

                    setPixel(index, existingColor.r, existingColor.g, existingColor.b);
                } else {
                    glm::vec3 color = colorFromUV(uvX, uvY, blockTypes.at(inventory.nodes[slotSelected].id).texture, 3);
                    setPixel(index, color.r, color.g, color.b);
                }
            }
        }
    }
    
}


void drawInventoryBackgroundToBuffer() {
    int squareWidth = invTexWidth;
    int startX = (int)((float)WIDTH * 0.291f);
    int startY = (int)(15 + yOffset * 70);
    if(drawingInv && loopFunc == &gameLoop) {
        for(int y = 0; y < squareWidth; y++) {
            for(int x = 0; x < squareWidth; x++) {
                int index = (startY + y) * WIDTH + (startX + x);
                float uvX = x / (float)squareWidth;
                float uvY = 1.0f - y / (float)squareWidth;

                
                    glm::vec4 color = colorWithAlphaFromUVImage(uvX, uvY, inventoryTexture, invTexWidth, invTexHeight);
                    if(color.a > 0) {
                        setPixel(index, color.r, color.g, color.b);
                    }
                        

            }
        }
    }
}


void bindMenuGeometry(GLuint vbo, const float *data, size_t dataSize) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, dataSize * sizeof(float), data, GL_STATIC_DRAW);

    GLint posAttrib = glGetAttribLocation(MENUSHADER, "pos");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    GLint texAttrib = glGetAttribLocation(MENUSHADER, "texcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));

    GLint elementIdAttrib = glGetAttribLocation(MENUSHADER, "elementid");
    glEnableVertexAttribArray(elementIdAttrib);
    glVertexAttribPointer(elementIdAttrib, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(4 * sizeof(float)));
}

void bindMenuGeometryNoUpload(GLuint vbo) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    GLint posAttrib = glGetAttribLocation(MENUSHADER, "pos");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    GLint texAttrib = glGetAttribLocation(MENUSHADER, "texcoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));

    GLint elementIdAttrib = glGetAttribLocation(MENUSHADER, "elementid");
    glEnableVertexAttribArray(elementIdAttrib);
    glVertexAttribPointer(elementIdAttrib, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(4 * sizeof(float)));
}

void drawInventoryForegroundElements() {
    glBindVertexArray(VAO2);
    glUseProgram(MENUSHADER);
    

    float invTileWidth = ((float)invTexWidth/WIDTH) / 3.5f;
    float invTileHeight = ((float)invTexWidth/HEIGHT) / 3.5f;
    float spacePixels = 4.0f/WIDTH;

    TextureFace invFace(3,0);

    if(drawingInv && loopFunc == &gameLoop) {
        

        
        

        static GLuint vbo;

        static std::vector<float> displayData;

        float invStartX = -0.372f;
        float invStartY = -0.48f;
        

        mousedOverElement = 0.0f;


            for(int k = 0; k < 3; k++) {
                for(int i = 0; i < 6; i++) {

                    glBindTexture(GL_TEXTURE_2D, MENUTEXTURE);

                    glDeleteBuffers(1, &vbo);
                    glGenBuffers(1, &vbo);


                    int invTileIndex = 6 + k * 6 + i;
                    displayData.clear();
                    displayData.insert(displayData.end(), {
                        invStartX + ((invTileWidth + spacePixels) * i),              invStartY+((invTileHeight + spacePixels) * k),                invFace.bl.x, invFace.bl.y, 1.0f * invTileIndex + 1.0f,
                        invStartX + ((invTileWidth + spacePixels) * i),              invStartY+((invTileHeight + spacePixels) * k)+invTileHeight,  invFace.bl.x, invFace.bl.y, 1.0f * invTileIndex + 1.0f,
                        invStartX + ((invTileWidth + spacePixels) * i)+invTileWidth, invStartY+((invTileHeight + spacePixels) * k)+invTileHeight,  invFace.bl.x, invFace.bl.y, 1.0f * invTileIndex + 1.0f,

                        invStartX + ((invTileWidth + spacePixels) * i)+invTileWidth, invStartY+((invTileHeight + spacePixels) * k)+invTileHeight,  invFace.bl.x, invFace.bl.y, 1.0f * invTileIndex + 1.0f,
                        invStartX + ((invTileWidth + spacePixels) * i)+invTileWidth, invStartY+((invTileHeight + spacePixels) * k),                invFace.bl.x, invFace.bl.y, 1.0f * invTileIndex + 1.0f,
                        invStartX + ((invTileWidth + spacePixels) * i),              invStartY+((invTileHeight + spacePixels) * k),                invFace.bl.x, invFace.bl.y, 1.0f * invTileIndex + 1.0f,
                    });
                    bindMenuGeometry(vbo, displayData.data(), displayData.size());

                    float screenPosx = (displayData[5] + 1.0f) * 0.5f;
                    float screenPosy = (1.0f - displayData[6]) * 0.5f;
                    float screenWidth = invTileWidth / 2;
                    float screenHeight = invTileHeight / 2;
                    float elementID = displayData[4];

                    double xpos, ypos;
                    glfwGetCursorPos(WINDOW, &xpos, &ypos);
                    if(xpos > screenPosx * SWIDTH &&
                    xpos < (screenPosx + screenWidth) * SWIDTH &&
                    ypos > screenPosy * SHEIGHT &&
                    ypos < (screenPosy + screenHeight) * SHEIGHT && !mouseCaptured)
                    {
                        mousedOverElement = elementID;
                    }

                    GLuint moeLocation = glGetUniformLocation(MENUSHADER, "mousedOverElement");
                    glUniform1f(moeLocation, mousedOverElement);
                    GLuint coeLocation = glGetUniformLocation(MENUSHADER, "clickedOnElement");
                    glUniform1f(coeLocation, clickedOnElement);

                    glDrawArrays(GL_TRIANGLES, 0, displayData.size()/5);

                }
            }
    }



    GLuint vbo2;

    

    std::vector<float> bottomInvDisplayData;

    float invStartX = -0.2f;
    float invStartY = -0.95f;

    for(int i = 0; i < 6; i++) {

        glDeleteBuffers(1, &vbo2);
        glGenBuffers(1, &vbo2);
        int invTileIndex = i;
        bottomInvDisplayData.clear();
        bottomInvDisplayData.insert(bottomInvDisplayData.end(), {
            invStartX + (((invTileWidth/2) + spacePixels) * i),                  invStartY,                    invFace.bl.x, invFace.bl.y, invTileIndex + 25.0f,
            invStartX + (((invTileWidth/2) + spacePixels) * i),                  invStartY+(invTileHeight/2),  invFace.bl.x, invFace.bl.y, invTileIndex + 25.0f,
            invStartX + (((invTileWidth/2) + spacePixels) * i)+(invTileWidth/2), invStartY+(invTileHeight/2),  invFace.bl.x, invFace.bl.y, invTileIndex + 25.0f,

            invStartX + (((invTileWidth/2) + spacePixels) * i)+(invTileWidth/2), invStartY+(invTileHeight/2),  invFace.bl.x, invFace.bl.y, invTileIndex + 25.0f,
            invStartX + (((invTileWidth/2) + spacePixels) * i)+(invTileWidth/2), invStartY,                    invFace.bl.x, invFace.bl.y, invTileIndex + 25.0f,
            invStartX + (((invTileWidth/2) + spacePixels) * i),                  invStartY,                    invFace.bl.x, invFace.bl.y, invTileIndex + 25.0f,
        });

        float screenPosx = (bottomInvDisplayData[5] + 1.0f) * 0.5f;
            float screenPosy = (1.0f - bottomInvDisplayData[6]) * 0.5f;
            float screenWidth = invTileWidth / 4;
            float screenHeight = invTileHeight / 4;
            float elementID = bottomInvDisplayData[4];

            double xpos, ypos;
            glfwGetCursorPos(WINDOW, &xpos, &ypos);
            if(xpos > screenPosx * SWIDTH &&
            xpos < (screenPosx + screenWidth) * SWIDTH &&
            ypos > screenPosy * SHEIGHT &&
            ypos < (screenPosy + screenHeight) * SHEIGHT &&
            elementID != -1.0f && !mouseCaptured) 
            {
                mousedOverElement = elementID;
            }

            GLuint moeLocation = glGetUniformLocation(MENUSHADER, "mousedOverElement");
            glUniform1f(moeLocation, mousedOverElement);
            GLuint coeLocation = glGetUniformLocation(MENUSHADER, "clickedOnElement");
            glUniform1f(coeLocation, clickedOnElement);

            glBindTexture(GL_TEXTURE_2D, MENUTEXTURE);

            bindMenuGeometry(vbo2, bottomInvDisplayData.data(), bottomInvDisplayData.size());

            glDrawArrays(GL_TRIANGLES, 0, bottomInvDisplayData.size()/5);

            


            if(inventory.nodes[invTileIndex].id != 0) {

                glDeleteBuffers(1, &vbo2);
                glGenBuffers(1, &vbo2);
                BlockType& bt = blockTypes.at(inventory.nodes[invTileIndex].id);
                if(bt.transparent) {
                    glBindTexture(GL_TEXTURE_2D, polyTextureTransparent);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 32, 32, GL_RGBA, GL_UNSIGNED_BYTE, bt.texture);
                } else {
                    glBindTexture(GL_TEXTURE_2D, polyTexture);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 32, 32, GL_RGB, GL_UNSIGNED_BYTE, bt.texture);
                }

                bottomInvDisplayData.clear();
                bottomInvDisplayData.insert(bottomInvDisplayData.end(), {
                    invStartX + (((invTileWidth/2) + spacePixels) * i),                  invStartY,                    0.0f, 1.0f, invTileIndex + 25.0f,
                    invStartX + (((invTileWidth/2) + spacePixels) * i),                  invStartY+(invTileHeight/2),  0.0f, 0.0f, invTileIndex + 25.0f,
                    invStartX + (((invTileWidth/2) + spacePixels) * i)+(invTileWidth/2), invStartY+(invTileHeight/2),  1.0f, 0.0f, invTileIndex + 25.0f,

                    invStartX + (((invTileWidth/2) + spacePixels) * i)+(invTileWidth/2), invStartY+(invTileHeight/2),  1.0f, 0.0f, invTileIndex + 25.0f,
                    invStartX + (((invTileWidth/2) + spacePixels) * i)+(invTileWidth/2), invStartY,                    1.0f, 1.0f, invTileIndex + 25.0f,
                    invStartX + (((invTileWidth/2) + spacePixels) * i),                  invStartY,                    0.0f, 1.0f, invTileIndex + 25.0f,
                });

                bindMenuGeometry(vbo2, bottomInvDisplayData.data(), bottomInvDisplayData.size());

                glDrawArrays(GL_TRIANGLES, 0, bottomInvDisplayData.size()/5);


                glBindTexture(GL_TEXTURE_2D, MENUTEXTURE);

                std::string countStr = std::to_string(inventory.nodes[invTileIndex].count);

                float lettersCount = std::strlen(countStr.c_str());

                glm::vec2 letterStart(invStartX, invStartY - 0.02f);

                GlyphFace glyph;

                static float letHeight = (32.0f/SHEIGHT);
                static float letWidth = (32.0f/SWIDTH);

                for(int l = 0; l < lettersCount; l++) {
                    glyph.setCharCode(static_cast<int>(countStr.c_str()[l]));
                    glm::vec2 thisLetterStart(letterStart.x + l*letWidth, letterStart.y);
                    bottomInvDisplayData.clear();
                    bottomInvDisplayData.insert(bottomInvDisplayData.end(), {
                        thisLetterStart.x+ (((invTileWidth/2) + spacePixels) * i), thisLetterStart.y,                     glyph.bl.x, glyph.bl.y, -1.0f,
                        thisLetterStart.x+ (((invTileWidth/2) + spacePixels) * i), thisLetterStart.y+letHeight,           glyph.tl.x, glyph.tl.y, -1.0f,
                        thisLetterStart.x+ (((invTileWidth/2) + spacePixels) * i)+letWidth, thisLetterStart.y+letHeight, glyph.tr.x, glyph.tr.y, -1.0f,

                        thisLetterStart.x+ (((invTileWidth/2) + spacePixels) * i)+letWidth, thisLetterStart.y+letHeight, glyph.tr.x, glyph.tr.y, -1.0f,
                        thisLetterStart.x+ (((invTileWidth/2) + spacePixels) * i)+letWidth, thisLetterStart.y,           glyph.br.x, glyph.br.y, -1.0f,
                        thisLetterStart.x+ (((invTileWidth/2) + spacePixels) * i), thisLetterStart.y,                     glyph.bl.x, glyph.bl.y, -1.0f
                    });
                }

                glDeleteBuffers(1, &vbo2);
                glGenBuffers(1, &vbo2);

                bindMenuGeometry(vbo2, bottomInvDisplayData.data(), bottomInvDisplayData.size());

                glDrawArrays(GL_TRIANGLES, 0, bottomInvDisplayData.size()/5);

            }
    }

    TextureFace select(4,0);

    glDeleteBuffers(1, &vbo2);
    glGenBuffers(1, &vbo2);

    bottomInvDisplayData.clear();
    bottomInvDisplayData.insert(bottomInvDisplayData.end(), {
            invStartX + (((invTileWidth/2) + spacePixels) * slotSelected),                  invStartY,                    select.bl.x, select.bl.y, -1.0f,
            invStartX + (((invTileWidth/2) + spacePixels) * slotSelected),                  invStartY+(invTileHeight/2),  select.tl.x, select.tl.y, -1.0f,
            invStartX + (((invTileWidth/2) + spacePixels) * slotSelected)+(invTileWidth/2), invStartY+(invTileHeight/2),  select.tr.x, select.tr.y, -1.0f,

            invStartX + (((invTileWidth/2) + spacePixels) * slotSelected)+(invTileWidth/2), invStartY+(invTileHeight/2),  select.tr.x, select.tr.y, -1.0f,
            invStartX + (((invTileWidth/2) + spacePixels) * slotSelected)+(invTileWidth/2), invStartY,                    select.br.x, invFace.br.y, -1.0f,
            invStartX + (((invTileWidth/2) + spacePixels) * slotSelected),                  invStartY,                    select.bl.x, select.bl.y, -1.0f,
        });

    glBindTexture(GL_TEXTURE_2D, MENUTEXTURE);

    bindMenuGeometry(vbo2, bottomInvDisplayData.data(), bottomInvDisplayData.size());

    glDrawArrays(GL_TRIANGLES, 0, bottomInvDisplayData.size()/5);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    SWIDTH = width;
    SHEIGHT = height;
    GUIButton::windowWidth = SWIDTH;
    GUIButton::windowHeight = SHEIGHT;
}




void drawGuiIfOpen() {
    glBindVertexArray(VAO2);

    if(currentGuiButtons != nullptr) {
        glUseProgram(MENUSHADER);
        glBindTexture(GL_TEXTURE_2D, MENUTEXTURE);

        mousedOverElement = 0.0f;

        for(GUIButton& button : *currentGuiButtons) {
            double xpos, ypos;
            glfwGetCursorPos(WINDOW, &xpos, &ypos);
            if(xpos > button.screenPos.x * SWIDTH &&
            xpos < (button.screenPos.x + button.screenWidth) * SWIDTH &&
            ypos > button.screenPos.y * SHEIGHT &&
            ypos < (button.screenPos.y + button.screenHeight) * SHEIGHT)
            {
                mousedOverElement = button.elementID;
            }

            if(mousedOverElement != lastMousedOverElement && mousedOverElement != 0.0f) {
                lastMousedOverElement = mousedOverElement;
                sfs.play(buttonHover);
            }
            

            if(!button.uploaded) {
                bindMenuGeometry(button.vbo, button.displayData.data(), button.displayData.size());
                button.uploaded = true;
            } else {
                bindMenuGeometryNoUpload(button.vbo);
            }
            glDrawArrays(GL_TRIANGLES, 0, button.displayData.size() / 5);
        }
        if(mousedOverElement == 0.0f) {
            lastMousedOverElement = 0.0f;
        }
        //std::cout << std::to_string(lastMousedOverElement) << " " << std::to_string(mousedOverElement) << "\n";

        GLuint moeLocation = glGetUniformLocation(MENUSHADER, "mousedOverElement");
        glUniform1f(moeLocation, mousedOverElement);
        GLuint coeLocation = glGetUniformLocation(MENUSHADER, "clickedOnElement");
        glUniform1f(coeLocation, clickedOnElement);
    }

}


void drawSplashScreen() {
    
    glBindVertexArray(VAO2);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);


    float splashImageWidth = 400;



    glm::vec2 splashLowerLeft(-splashImageWidth/SWIDTH, -splashImageWidth/SHEIGHT);
    float relHeight = splashImageWidth/(SHEIGHT/2);
    float relWidth = splashImageWidth/(SWIDTH/2);


    std::vector<float> splashDisplayData = {
        splashLowerLeft.x, splashLowerLeft.y,                    0.0f, 1.0f,   -1.0f,
        splashLowerLeft.x, splashLowerLeft.y+relHeight,          0.0f, 0.0f,   -1.0f,
        splashLowerLeft.x+relWidth, splashLowerLeft.y+relHeight, 1.0f, 0.0f,   -1.0f,

        splashLowerLeft.x+relWidth, splashLowerLeft.y+relHeight, 1.0f, 0.0f,   -1.0f,
        splashLowerLeft.x+relWidth, splashLowerLeft.y,           1.0f, 1.0f,   -1.0f,
        splashLowerLeft.x, splashLowerLeft.y,                    0.0f, 1.0f,   -1.0f
    };



    glUseProgram(MENUSHADER);

   
    glBindTexture(GL_TEXTURE_2D, SPLASHTEXTURE);


    static GLuint vbo = 0;
    if(vbo == 0) {
        glGenBuffers(1, &vbo);
    }

        bindMenuGeometry(vbo, 
        splashDisplayData.data(),
        splashDisplayData.size());


    glDrawArrays(GL_TRIANGLES, 0, splashDisplayData.size()/5);

}




//Uncomment this stuff to remove console when done:
//#include <Windows.h>
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
int main() {

    PaError err;

    Pa_Initialize();

    audioData1 = loadAudioFile("assets/song1.mp3");
    audioData2 = loadAudioFile("assets/song2.mp3");
    audioData3 = loadAudioFile("assets/song3.mp3");


    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice(); // Default output device
    outputParameters.channelCount = channels; // Stereo output
    outputParameters.sampleFormat = paFloat32; // 32-bit floating-point output
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&musicStream,
                        NULL, // No input parameters, as we're only playing audio
                        &outputParameters, // Output parameters
                        sampleRate, // Sample rate
                        256, // Frames per buffer
                        paClipOff, // Stream flags
                        musicCallback, // Callback function
                        NULL); // User data
    if (err != paNoError) {
        std::cout << "Error opening musicStream" << Pa_GetErrorText(err) << "\n";
    }

    err = Pa_OpenStream(&sfxStream,
                        NULL, // No input parameters, as we're only playing audio
                        &outputParameters, // Output parameters
                        sampleRate, // Sample rate
                        256, // Frames per buffer
                        paClipOff, // Stream flags
                        sfxCallback, // Callback function
                        NULL); // User data
    if (err != paNoError) {
        std::cout << "Error opening sfxStream" << Pa_GetErrorText(err) << "\n";
    }

    err = Pa_StartStream(musicStream);
    if (err != paNoError) {
        // Handle error
    }

    err = Pa_StartStream(sfxStream);
    if (err != paNoError) {
        // Handle error
    }


    menuLoop = [](){
        
            
        glClearColor(0.0f,0.0f,0.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        updateTime();

        glBindVertexArray(VAO);
        glUseProgram(SHADER_PROG1);
        glBindTexture(GL_TEXTURE_2D, TEXTURE_ID);

        GLuint yoloc = glGetUniformLocation(SHADER_PROG1, "yOffset");
        glUniform1f(yoloc, yOffset);


        glm::ivec3 firstOne;
        for(int i = 0; i < HEIGHT*3; i++) {
            for(int k = 0; k < WIDTH; k+= 3) {




                int posOrNeg = ((float)rand()/RAND_MAX) > 0.5 ? 1 : -1;
                int r = ((float)rand()/RAND_MAX) * 2 * posOrNeg;
                PIXELS[i*WIDTH + k] = std::min(130, std::max(50, PIXELS[i*WIDTH + k] + r));
                PIXELS[i*WIDTH + k+1] = std::min(130, std::max(50, PIXELS[i*WIDTH + k+1] + r));
                PIXELS[i*WIDTH + k+2] = std::min(130, std::max(50, PIXELS[i*WIDTH + k+2] + r));

                
                
            }
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, PIXELS);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        drawGuiIfOpen();



        float logoImageWidth = 600;



            glm::vec2 logoLowerLeft(-logoImageWidth/SWIDTH, -logoImageWidth/SHEIGHT);
            float relHeight = logoImageWidth/(SHEIGHT/2);
            float relWidth = logoImageWidth/(SWIDTH/2);

            float shiftUp = 0.5f;


            std::vector<float> logoDisplayData;

            float numLetters = 6.0f;

            float oneOverNine = 1.0f/numLetters;

            for(int i = 0; i < numLetters; ++i) {
                logoDisplayData.insert(logoDisplayData.end(), {
                    logoLowerLeft.x + i*(relWidth/numLetters),               logoLowerLeft.y + shiftUp,                    0.0f + i*oneOverNine,      1.0f,   -67.0f - (i*1.0f),
                    logoLowerLeft.x + i*(relWidth/numLetters),               logoLowerLeft.y+relHeight+ shiftUp,          0.0f + i*oneOverNine,       0.0f,   -67.0f - (i*1.0f),
                    logoLowerLeft.x +(relWidth/numLetters) + i*(relWidth/numLetters), logoLowerLeft.y+relHeight+ shiftUp,         oneOverNine + i*oneOverNine, 0.0f,   -67.0f - (i*1.0f),

                    logoLowerLeft.x +(relWidth/numLetters) + i*(relWidth/numLetters), logoLowerLeft.y+relHeight+ shiftUp,          oneOverNine + i*oneOverNine, 0.0f,   -67.0f - (i*1.0f),
                    logoLowerLeft.x +(relWidth/numLetters) + i*(relWidth/numLetters), logoLowerLeft.y+ shiftUp,                    oneOverNine + i*oneOverNine, 1.0f,   -67.0f - (i*1.0f),
                    logoLowerLeft.x + i*(relWidth/numLetters),               logoLowerLeft.y+ shiftUp,                    0.0f + i*oneOverNine,        1.0f,   -67.0f - (i*1.0f)
                });
            }

            GLuint timeLocation = glGetUniformLocation(MENUSHADER, "time");
            glUniform1f(timeLocation, static_cast<float>(std::fmod(glfwGetTime()-2.0, 6.0f)));

        
            glBindTexture(GL_TEXTURE_2D, LOGOTEXTURE);


            static GLuint vbo = 0;
            if(vbo == 0) {
                glGenBuffers(1, &vbo);
            }

                bindMenuGeometry(vbo, 
                logoDisplayData.data(),
                logoDisplayData.size());


            glDrawArrays(GL_TRIANGLES, 0, logoDisplayData.size()/5);
            const char* message = "Version 0.0.1b";

            std::vector<float> displayData;


            float letHeight = (32.0f/SHEIGHT);
            float letWidth = (32.0f/SWIDTH);
            float lettersCount = std::strlen(message);
            float totletwid = letWidth * lettersCount;
            glm::vec2 letterStart(-totletwid/2, -letHeight/2 + 0.2f);

            GlyphFace glyph;

            for(int i = 0; i < lettersCount; i++) {
                glyph.setCharCode(static_cast<int>(message[i]));
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

            static GLuint vbo2 = 0;
            if(vbo2 == 0) {
                glGenBuffers(1, &vbo2);
            }

                bindMenuGeometry(vbo2, 
                displayData.data(),
                displayData.size());

            glBindTexture(GL_TEXTURE_2D, MENUTEXTURE);
            glDrawArrays(GL_TRIANGLES, 0, displayData.size()/5);

    };




    gameLoop = [](){
        
            glBindVertexArray(VAO);
            glUseProgram(SHADER_PROG1);
            glBindTexture(GL_TEXTURE_2D, TEXTURE_ID);

            GLuint yoloc = glGetUniformLocation(SHADER_PROG1, "yOffset");
            glUniform1f(yoloc, yOffset);

            glClearColor(0.0f,0.0f,0.0f,1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            updateTime();
            stepGoingDown();
            stepJumping();
            castRaysFromCamera();

            if(sampleMap(viewedBlock.x, viewedBlock.y) == 55) {
                textView.setTextNode("ShopInfo", "Press K to enter the shop!", glm::vec2(-0.5f, 0.5f));
            } else {
                textView.removeTextNode("ShopInfo");
            }

            drawSelectedBlock();
            drawInventoryBackgroundToBuffer();

            glBindTexture(GL_TEXTURE_2D, TEXTURE_ID);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, PIXELS);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            drawInventoryForegroundElements();

            drawGuiIfOpen();

            glBindVertexArray(VAO2);
            
            textView.display();
    };

    splashLoop = [](){
        static float timer = 0.0f;
        drawSplashScreen();
        updateTime();
        if(timer > 2.0f) {
            goToMainMenu();
        } else {
            timer += deltaTime;
        }
    };



    std::cout << std::filesystem::current_path() << "\n";



    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Necessary for macOS
    #endif
    if (!(WINDOW = glfwCreateWindow(SWIDTH, SHEIGHT, "Mimodo", NULL, NULL))) {
        glfwTerminate();
        return EXIT_FAILURE;
    }


    glfwMakeContextCurrent(WINDOW);
    glewInit();
    // glViewport(0, 0, SWIDTH, SHEIGHT);
    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    // glFrontFace(GL_CW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glDepthFunc(GL_LESS);

    glfwSetMouseButtonCallback(WINDOW, mouse_button_callback);
    glfwSetCursorPosCallback(WINDOW, cursor_position_callback);
    glfwSetKeyCallback(WINDOW, key_callback);
    glfwSetScrollCallback(WINDOW, scroll_callback);
    glfwSetFramebufferSizeCallback(WINDOW, framebuffer_size_callback);
    //"World" Shader
    std::string vertexShaderSrc;
    std::string fragmentShaderSrc;
    load_text("shad/vert.glsl", vertexShaderSrc);
    load_text("shad/frag.glsl", fragmentShaderSrc);
    const GLchar *vertexGLChars = vertexShaderSrc.c_str();
    const GLchar *fragGLChars = fragmentShaderSrc.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexGLChars, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragGLChars, NULL);
    glCompileShader(fragmentShader);

    SHADER_PROG1 = glCreateProgram();
    glAttachShader(SHADER_PROG1, vertexShader);
    glAttachShader(SHADER_PROG1, fragmentShader);
    glLinkProgram(SHADER_PROG1);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    vertexShaderSrc = "";
    fragmentShaderSrc = "";
    //Menu Shader

    load_text("shad/menuVert.glsl", vertexShaderSrc);
    load_text("shad/menuFrag.glsl", fragmentShaderSrc);
    vertexGLChars = vertexShaderSrc.c_str();
    fragGLChars = fragmentShaderSrc.c_str();

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexGLChars, NULL);
    glCompileShader(vertexShader);

    // Check for vertex shader compilation errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::VERTEX_SHADER_COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragGLChars, NULL);
    glCompileShader(fragmentShader);

    // Check for fragment shader compilation errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::FRAGMENT_SHADER_COMPILATION_FAILED\n" << infoLog << std::endl;
    }


    MENUSHADER = glCreateProgram();
    glAttachShader(MENUSHADER, vertexShader);
    glAttachShader(MENUSHADER, fragmentShader);
    glLinkProgram(MENUSHADER);

    // Check for shader program linking errors
    glGetProgramiv(MENUSHADER, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(MENUSHADER, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER_PROGRAM_LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenTextures(1, &MENUTEXTURE);
    glBindTexture(GL_TEXTURE_2D, MENUTEXTURE);

    int width, height, nrchan;
    unsigned char *data = stbi_load("assets/gui.png", &width, &height, &nrchan, 0);
    if (data)
    {
        std::cout << "channels: " << std::to_string(nrchan) << "\n";
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else
    {
        std::cout << "Failed to load texture menutexture" << std::endl;
    }
    stbi_image_free(data);


    glGenTextures(1, &LOGOTEXTURE);
    glBindTexture(GL_TEXTURE_2D, LOGOTEXTURE);

    data = stbi_load("assets/logo.png", &width, &height, &nrchan, 0);
    if (data)
    {
        std::cout << "channels: " << std::to_string(nrchan) << "\n";
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else
    {
        std::cout << "Failed to load texture LOGOTEXTURE" << std::endl;
    }
    stbi_image_free(data);

    glGenTextures(1, &SPLASHTEXTURE);
    glBindTexture(GL_TEXTURE_2D, SPLASHTEXTURE);

    data = stbi_load("assets/splash.png", &width, &height, &nrchan, 0);
    if (data)
    {
        std::cout << "channels: " << std::to_string(nrchan) << "\n";
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else
    {
        std::cout << "Failed to load texture SPLASH" << std::endl;
    }
    stbi_image_free(data);


    glGenTextures(1, &TEXTURE_ID);
    glBindTexture(GL_TEXTURE_2D, TEXTURE_ID);

    int w, h, c;
    PIXELS = stbi_load("assets/firstback.png", &w, &h, &c, 0);
    if (!PIXELS)
    {
        std::cout << "Failed to load texture firstback" << std::endl;
    }
    std::cout << std::to_string(w) << " " << std::to_string(h) << " " << std::to_string(c) << "\n";

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, PIXELS);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    

    //The quad that the entire screen is drawn on
    float quadVertices[] = {
    // positions   // texture coords
    -1.0f - MAXIMUMYSHEAR,  1.0f + MAXIMUMYSHEAR,  0.0f, 0.0f, // Top Left
    -1.0f- MAXIMUMYSHEAR, -1.0f - MAXIMUMYSHEAR,  0.0f, 1.0f, // Bottom Left
     1.0f + MAXIMUMYSHEAR, -1.0f - MAXIMUMYSHEAR,  1.0f, 1.0f, // Bottom Right

    -1.0f - MAXIMUMYSHEAR,  1.0f + MAXIMUMYSHEAR,  0.0f, 0.0f, // Top Left
     1.0f + MAXIMUMYSHEAR, -1.0f - MAXIMUMYSHEAR,  1.0f, 1.0f, // Bottom Right
     1.0f + MAXIMUMYSHEAR,  1.0f + MAXIMUMYSHEAR,  1.0f, 0.0f  // Top Right
    };


    glGenVertexArrays(1, &VAO);
    glGenVertexArrays(1, &VAO2);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // position attribute
    GLint pos_attrib = glGetAttribLocation(SHADER_PROG1, "pos");
    glEnableVertexAttribArray(pos_attrib);
    glVertexAttribPointer(pos_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // texture coord attribute
    GLint tex_attrib = glGetAttribLocation(SHADER_PROG1, "texcoord");
    glEnableVertexAttribArray(tex_attrib);
    glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    

    loadTexture();
    loadSavedCameraPosition();
    loadMap(LAYER);
    //cleanMap();

    
    blockTypesOrdered = std::vector<std::pair<int, BlockType>>(blockTypes.begin(), blockTypes.end());
    
    textView.create();

    textView.windowHeight = SHEIGHT;
    textView.windowWidth = SWIDTH;
    textView.setTextNode("Test", "Build Mode OFF", glm::vec2(0.4f,-0.4f));

    GUIButton::windowWidth = SWIDTH;
    GUIButton::windowHeight = SHEIGHT;

    glBindVertexArray(VAO2);
    
    startMainTheme();
    loopFunc = &splashLoop;
   

    inventory.fakeLoadInventory();

    while(!glfwWindowShouldClose(WINDOW)) {
        (*loopFunc)();
        glfwSwapBuffers(WINDOW);
        glfwPollEvents();
    }

    //Save the map
    //int saveResult = stbi_write_bmp("assets/map.bmp", MAPWIDTH, MAPWIDTH, 1, MAP);
    for(int i = 0; i <= FURTHESTLOADED; i++) {
        std::string mapPath("maps/map");
        mapPath += std::to_string(i) + ".bmp";
        int saveResult = stbi_write_bmp(mapPath.c_str(), MAPWIDTH, MAPWIDTH, 1, MAPS[i]);
    }
    if(!INSHOP) {
        saveCameraPosition();
    }
    

    glfwDestroyWindow(WINDOW);
    glfwTerminate();

    Pa_StopStream(sfxStream);
    Pa_CloseStream(sfxStream);

    Pa_StopStream(musicStream);
    Pa_CloseStream(musicStream);
    Pa_Terminate();

    return EXIT_SUCCESS;
}