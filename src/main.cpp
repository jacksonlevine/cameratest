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

bool GOINGDOWN = false;
bool GOINGUP = false;

bool FUCKEDUP = false;

int FURTHESTLOADED = -1;


int LAYER = 0;


glm::vec2 cameraPosition = glm::vec2(0,0);
float cameraAngle = 0.0f;

int MAPWIDTH = 100;

GLubyte * MAP;

std::vector<GLubyte *> MAPS;

void loadMap(int layer) {
    int width, height, nrChannels;
    if(layer > FURTHESTLOADED) {
        FURTHESTLOADED = layer;
        std::string mapPath("assets/maps/map");
        mapPath += std::to_string(layer) + ".bmp";
        std::cout << "Checking for: " << mapPath << "\n";
        if(std::filesystem::exists(mapPath)) {
            std::cout << "Found it!" << "\n";
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
    }
}

TextView textView;


struct BlockType {
    GLubyte *texture;
    bool transparent;
    bool isCrossMesh;
};


glm::vec2 viewedBlock;


const int SWIDTH = 1280;
const int SHEIGHT = 720;

int blockTypeSelected = 0;


int forward = 0;
int backward = 0;
int left = 0;
int right = 0;


float FLOORLEVEL = 1.0f;

GLFWwindow * WINDOW;

GLuint SHADER_PROG1;
GLuint TEXTURE_ID;

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

void updateTime() {
    double currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
}




glm::ivec3 BACKGROUNDCOLOR = glm::ivec3(90, 0, 150);

GLubyte PIXELS[WIDTH * HEIGHT * NUMCHANNELS];

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

std::map<int, BlockType> blockTypes;
std::vector<std::pair<int, BlockType>> blockTypesOrdered;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    blockTypeSelected = std::max(0, std::min((int)blockTypesOrdered.size()-1, (int)(blockTypeSelected + yoffset)));
}


void loadTexture() {

    int width, height;
    stoneWallTexture = stbi_load("assets/stonewall.png", &width, &height, &nrChannels, 0);
    if (stoneWallTexture)
    {
        std::cout << "channels: " << nrChannels << "\n";
    }
    else
    {
        std::cout << "Failed to load texture stonewall" << std::endl;
    }

    MAP = stbi_load("assets/map.bmp", &MAPWIDTH, &height, &nrMapChannels, 1);
    assert(MAPWIDTH == height);
    assert(nrMapChannels == 1);
    if (MAP)
    {
        std::cout << "channels: " << nrMapChannels << "\n";
    }
    else
    {
        std::cout << "Failed to load texture map" << std::endl;
    }

    floorTexture = stbi_load("assets/floor.png", &width, &height, &nrChannels, 0);
    if (floorTexture)
    {
        std::cout << "channels: " << nrChannels << "\n";
    }
    else
    {
        std::cout << "Failed to load texture floor" << std::endl;
    }

    plyWoodTexture = stbi_load("assets/plywood.png", &width, &height, &nrChannels, 0);
    if (plyWoodTexture)
    {
        std::cout << "channels: " << nrChannels << "\n";
    }
    else
    {
        std::cout << "Failed to load texture plywood" << std::endl;
    }

    glassTexture = stbi_load("assets/glass.png", &width, &height, &nrChannels, 0);
    if (glassTexture)
    {
        std::cout << "glass channels: " << nrChannels << "\n";
    }
    else
    {
        std::cout << "Failed to load texture glassTexture" << std::endl;
    }

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
        if(blockTypes.find(MAPS[LAYER][test]) == blockTypes.end()) {
                    //std::cout << "cleaning this \n";
                    MAPS[LAYER][test] = 0;
        }
        return MAPS[LAYER][test];
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

    if(!GOINGDOWN && !GOINGUP) {
        if(right) {
            
            proposedPosition += directionFromAngle(cameraAngle+((std::acos(-1.0f)/2.0f)))*speedMult*deltaTime;
        }
        if(left) {
            proposedPosition += directionFromAngle(cameraAngle-((std::acos(-1.0f)/2.0f)))*speedMult*deltaTime;
        }
        if(forward) {
            proposedPosition += directionFromAngle(cameraAngle)*speedMult*deltaTime;
        }
        if(backward) {
            proposedPosition -= directionFromAngle(cameraAngle)*speedMult*deltaTime;
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
                if(LAYER == 0) {
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
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if(mouseCaptured) {
            if(BUILDMODE) {
                int mapInd = mapIndexFromCoord(viewedBlock.x, viewedBlock.y);
                if(mapInd != -1) {
                    MAPS[LAYER][mapInd] = 0;
                }
            }
            
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseCaptured = true;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if(mouseCaptured) {
            if(BUILDMODE) {
                int mapInd = mapIndexFromCoord(viewedBlock.x, viewedBlock.y);
                if(mapInd != -1) {
                    if(MAPS[LAYER][mapInd] == 0) {
                        if(blockTypesOrdered[blockTypeSelected].first == 52) {
                            loadMap(LAYER + 1);
                            if(MAPS[LAYER+1][mapInd] == 0 || MAPS[LAYER+1][mapInd] == 52) {
                                MAPS[LAYER][mapInd] = 52;
                            }
                        } else {
                            MAPS[LAYER][mapInd] = blockTypesOrdered[blockTypeSelected].first;
                        }
                        
                    }
                }
            } else {
                int mapInd = mapIndexFromCoord(viewedBlock.x, viewedBlock.y);
               
                if(mapInd != -1) {
                    if(MAPS[LAYER][mapInd] == 5) {
                        MAPS[LAYER][mapInd] = 6;
                    } else 
                    if(MAPS[LAYER][mapInd] == 6) {
                        MAPS[LAYER][mapInd] = 5;
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

bool JUMPKEYHELD = false;
bool JUMPING = false;

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
                jumper = 0.0f;
                jumpHeightReached = false;
                JUMPING = false;
            }
        }

        FLOORLEVEL = std::min(1.0f, 1.0f - jumper);
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
    if (key == GLFW_KEY_B && action == 1) {
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
        } else 
        if(!mouseCaptured && action == 1){
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseCaptured = true;
        }
    }
    if((key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) && action == 1 && !GOINGUP) {
        glm::ivec2 cameraTile(std::round(cameraPosition.x), std::round(cameraPosition.y));
        int tile = sampleMap(cameraTile.x, cameraTile.y);
        if(tile == 52) {
            loadMap(LAYER+1);
            if(MAPS[LAYER+1][mapIndexFromCoord(cameraTile.x, cameraTile.y)] == 0 || MAPS[LAYER+1][mapIndexFromCoord(cameraTile.x, cameraTile.y)] == 52)
            GOINGDOWN = true;
        }
    }
    if(key == GLFW_KEY_SPACE) {
        glm::ivec2 cameraTile(std::round(cameraPosition.x), std::round(cameraPosition.y));
        int tile = sampleMap(cameraTile.x, cameraTile.y);
        if(tile == 52 && LAYER > 0) {
            if(action == 1 && !GOINGDOWN) {
            
            
                loadMap(LAYER-1);
                if(MAPS[LAYER-1][mapIndexFromCoord(cameraTile.x, cameraTile.y)] == 0 || MAPS[LAYER-1][mapIndexFromCoord(cameraTile.x, cameraTile.y)] == 52)
                GOINGUP = true;
            }
        } else {
            JUMPKEYHELD = action ? true : false;
        }
        
    }
    
    if(key == GLFW_KEY_EQUAL && action == 1) {
        FUCKEDUP = !FUCKEDUP;
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
    int squareWidth = 10;
    int startX = (int)((float)WIDTH * 0.75f);
    int startY = (int)(((float)HEIGHT + yOffset * 100) * 0.75f);

    for(int y = 0; y < squareWidth; y++) {
        for(int x = 0; x < squareWidth; x++) {
            int index = (startY + y) * WIDTH + (startX + x);
            float uvX = x / (float)squareWidth;
            float uvY = 1.0f - y / (float)squareWidth;

            if(blockTypesOrdered[blockTypeSelected].second.transparent) {
                glm::vec4 color = colorWithAlphaFromUV(uvX, uvY, blockTypesOrdered[blockTypeSelected].second.texture);
                glm::vec3 existingColor = getPixelVal(index);

                existingColor.r = std::min(255, (int)(existingColor.r + color.r * (color.a / 255.0f)));
                existingColor.g = std::min(255, (int)(existingColor.g + color.g * (color.a / 255.0f)));
                existingColor.b = std::min(255, (int)(existingColor.b + color.b * (color.a / 255.0f)));

                setPixel(index, existingColor.r, existingColor.g, existingColor.b);
            } else {
                glm::vec3 color = colorFromUV(uvX, uvY, blockTypesOrdered[blockTypeSelected].second.texture, 3);
                setPixel(index, color.r, color.g, color.b);
            }
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}




//Uncomment this stuff to remove console when done:
//#include <Windows.h>
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
int main() {






    glfwInit();
    if (!(WINDOW = glfwCreateWindow(SWIDTH, SHEIGHT, "RayTest", NULL, NULL))) {
        glfwTerminate();
        return EXIT_FAILURE;
    }


    glfwMakeContextCurrent(WINDOW);
    glewInit();

    glfwSetMouseButtonCallback(WINDOW, mouse_button_callback);
    glfwSetCursorPosCallback(WINDOW, cursor_position_callback);
    glfwSetKeyCallback(WINDOW, key_callback);
    glfwSetScrollCallback(WINDOW, scroll_callback);
    glfwSetFramebufferSizeCallback(WINDOW, framebuffer_size_callback);

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

    glGenTextures(1, &TEXTURE_ID);
    glBindTexture(GL_TEXTURE_2D, TEXTURE_ID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

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


    GLuint VAO, VAO2, VBO;
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


    while(!glfwWindowShouldClose(WINDOW)) {
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

        drawSelectedBlock();

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, PIXELS);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(VAO2);
        
        textView.display();


        glfwSwapBuffers(WINDOW);
        glfwPollEvents();
    }

    //Save the map
    //int saveResult = stbi_write_bmp("assets/map.bmp", MAPWIDTH, MAPWIDTH, 1, MAP);
    for(int i = 0; i <= FURTHESTLOADED; i++) {
        std::string mapPath("assets/maps/map");
        mapPath += std::to_string(i) + ".bmp";
        int saveResult = stbi_write_bmp(mapPath.c_str(), MAPWIDTH, MAPWIDTH, 1, MAPS[i]);
    }
    saveCameraPosition();

    glfwDestroyWindow(WINDOW);
    glfwTerminate();

    return EXIT_SUCCESS;
}