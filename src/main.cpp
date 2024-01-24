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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "collcage.h"

#include <map>
#include <stack>

struct BlockType {
    GLubyte *texture;
    bool transparent;
};


glm::vec2 viewedBlock;


const int SWIDTH = 1280;
const int SHEIGHT = 720;


int forward = 0;
int backward = 0;
int left = 0;
int right = 0;

GLFWwindow * WINDOW;

GLuint SHADER_PROG1;
GLuint TEXTURE_ID;


float MAXIMUMYSHEAR = 0.0f;




float VIEWDISTANCE = 10.0f;

const int WIDTH = 320;
const int HEIGHT = 180;


float WALLHEIGHT = (float)HEIGHT;

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
GLubyte* plyWoodTexture;
GLubyte* glassTexture;


std::map<int, BlockType> blockTypes;


int MAPWIDTH;

GLubyte * MAP;

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

    blockTypes = {
    {255, BlockType{
        stoneWallTexture,
        false
    }},
    {100, BlockType{
        plyWoodTexture,
        false
    }},
    {150, BlockType{
        glassTexture,
        true
    }}
};
}



glm::ivec3 colorFromUV(float uvX, float uvY, GLubyte* texture, int nrChannels) {
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

glm::vec2 cameraPosition = glm::vec2(0,0);
float cameraAngle = 0.0f;

void setPixel(int ind, GLubyte r,GLubyte g,GLubyte b) {
    PIXELS[ind*3] = r;
    PIXELS[ind*3 + 1] = g;
    PIXELS[ind*3 + 2] = b;
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
        return MAP[test];
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

float speedMult = 2.5f;

LilCollisionCage collcage([](int x, int y){
    int samp = sampleMap(x,y);
    return samp != -1 && samp != 0;
});

BoundingBox user(cameraPosition, glm::vec2(0,0));

struct HitPoint {
    int type;
    float travel;
    glm::vec2 testSpot;
    glm::vec2 hitSpot;
    glm::vec2 lastSpotBeforeHit;
};

void castRaysFromCamera() {

    collcage.update_readings(cameraPosition);

    glm::vec2 proposedPosition = cameraPosition;

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
        float angle = ((((row - (HEIGHT / 2)) / (HEIGHT/2.0f) + 1.0f) * std::acos(-1.0f)) / 4.0f) - std::acos(-1.0f)/4.0f;

        glm::vec3 rayDir(cameraDirection.x, angle, cameraDirection.y);
        rayDir = glm::normalize(rayDir);

        float travel = 0.0f;

        glm::vec3 testSpot;

        bool hit = false;
        
        static float RAYSTEP = 0.01f;

        for(float i = 0; i < VIEWDISTANCE; i+= RAYSTEP) {
            testSpot = glm::vec3(cameraPosition.x, cameraY, cameraPosition.y) + rayDir * i;

            if(testSpot.y >= 1.0f || testSpot.y <= 0.0f) {
                hit = true;
                break;
            }
            travel += RAYSTEP;
        }

        if(hit) {

            static float FOV = std::acos(-1.0f)/2.0f/* your field of view in radians */;




            static float anglePerPixel = FOV / WIDTH;

            
            for(int i = 0; i < WIDTH; i++) {

                int ind = row * WIDTH + i;

                float angleOffset = (i - WIDTH / 2.0f) * anglePerPixel;

                glm::vec2 directionForPixel = directionFromAngle(cameraAngle + angleOffset);

                glm::vec3 worldPosition = glm::vec3(cameraPosition.x, cameraY, cameraPosition.y) + glm::vec3(directionForPixel.x, 0, directionForPixel.y) * travel * 1.5f;

                float uvX = std::abs(std::fmod(worldPosition.x, 1.0f));
                float uvY = std::abs(std::fmod(worldPosition.z, 1.0f));

                glm::ivec3 color = colorFromUV(uvX, uvY, floorTexture, 3);
                
                setPixel(ind, glm::mix(color.r, BACKGROUNDCOLOR.r, std::min(1.0f, travel*1.5f/VIEWDISTANCE)), glm::mix(color.g, BACKGROUNDCOLOR.g, std::min(1.0f, travel*1.5f/VIEWDISTANCE)), glm::mix(color.b, BACKGROUNDCOLOR.b, std::min(1.0f, travel*1.5f/VIEWDISTANCE)));
            }
            //std::cout << travel << "\n";
        }

    }


    for(int col = 0; col < WIDTH; col++) {

        std::stack<HitPoint> drawQueue;

        float angle = ((((col - (WIDTH / 2)) / (WIDTH/2.0f) + 1.0f) * std::acos(-1.0f)) / 4.0f) - std::acos(-1.0f)/4.0f;

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
        for(float i = 0; i < VIEWDISTANCE; i += RAYSTEP) {
             testSpot += (rayDir * RAYSTEP);
            int sampled = sampleMap(std::round(testSpot.x), std::round(testSpot.y));
            
            hit = sampled;
            travel+=RAYSTEP;

            if(sampled != 0) {

                
                if(sampled != -1) {
                    float rolledBack = 0.0f;
                    //std::cout << "travel was " << travel << "\n";
                    hitSpot = glm::vec2(std::round(testSpot.x), std::round(testSpot.y));
                    if(col == WIDTH/2) {
                        viewedBlock = hitSpot;
                    }
                    while (sampleMap(std::round(testSpot.x), std::round(testSpot.y)) != 0 && sampleMap(std::round(testSpot.x), std::round(testSpot.y)) != -1) {
                        testSpot -= rayDir*0.001f;
                        travel -= 0.001f;
                        rolledBack += 0.001f;
                    }

                    testSpot += rayDir*0.005f;
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
                        while (sampleMap(std::round(testSpot.x), std::round(testSpot.y)) == sampled) {
                            testSpot += rayDir*0.01f;
                            travel += 0.01f;
                            i+=0.01f;
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



        //std::cout << drawQueue.size() << "\n";
        while(!drawQueue.empty())
        {
            HitPoint h = drawQueue.top();

            drawQueue.pop();

            if(h.type == 0 || h.type == -1) {
                int height = ( WALLHEIGHT / h.travel) / 2.0f;

                int trav = 0;
                for(int i = HEIGHT/2; i < HEIGHT; i++) {
                    int ind = pixelIndexFromCoord(col, i);
                    if(ind != -1) {
                        float uvY = 0.5f - (((float)trav/height) / 2.0f);
                        if(trav < height) {
                            

                            setPixel(ind, BACKGROUNDCOLOR.r, BACKGROUNDCOLOR.g, BACKGROUNDCOLOR.b);
                        } else {
                            break;
                        }
                        
                    }
                    trav++;
                }
                trav = 0;
                for(int i = HEIGHT/2; i > -1; i--) {
                    int ind = pixelIndexFromCoord(col, i);
                    if(ind != -1) {
                        float uvY = 0.5f + (((float)trav/height) / 2.0f);
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

                if (diffX > diffZ) {
                    uvX = std::abs(zmod); 
                } else {
                    uvX = std::abs(xmod); 
                }


                //std::cout << "hit on col " << col << " with travel " << travel << "\n";'

                int height = ( WALLHEIGHT / h.travel) / 2.0f;

                int trav = 0;
                for(int i = HEIGHT/2; i < HEIGHT; i++) {
                    int ind = pixelIndexFromCoord(col, i);
                    if(ind != -1) {
                        float uvY = 0.5f - (((float)trav/height) / 2.0f);
                        if(trav < height) {
                            

                            glm::ivec3 color = colorFromUV(uvX, uvY, blockHere.texture, blockHere.transparent ? 4 : 3);
                            if(color.r != -1) {
                                setPixel(ind, glm::mix(color.r, BACKGROUNDCOLOR.r, h.travel/VIEWDISTANCE), glm::mix(color.g, BACKGROUNDCOLOR.g, h.travel/VIEWDISTANCE), glm::mix(color.b, BACKGROUNDCOLOR.b, h.travel/VIEWDISTANCE));
                            }
                            
                        } else {
                            break;
                        }
                        
                    }
                    trav++;
                }
                trav = 0;
                for(int i = HEIGHT/2; i > -1; i--) {
                    int ind = pixelIndexFromCoord(col, i);
                    if(ind != -1) {
                        float uvY = 0.5f + (((float)trav/height) / 2.0f);
                        if(trav < height) {
                            glm::ivec3 color = colorFromUV(uvX, uvY, blockHere.texture, blockHere.transparent ? 4 : 3);
                            if(color.r != -1)
                            {   
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

bool mouseCaptured = false;
bool firstMouse = true;


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        int mapInd = mapIndexFromCoord(viewedBlock.x, viewedBlock.y);
        if(mapInd != -1) {
            MAP[mapInd] = 0;
        }

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
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
        
}



//Uncomment this stuff to remove console when done:
//#include <Windows.h>
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
int main() {






    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    if (!(WINDOW = glfwCreateWindow(SWIDTH, SHEIGHT, "RayTest", NULL, NULL))) {
        glfwTerminate();
        return EXIT_FAILURE;
    }


    glfwMakeContextCurrent(WINDOW);
    glewInit();

    glfwSetMouseButtonCallback(WINDOW, mouse_button_callback);
    glfwSetCursorPosCallback(WINDOW, cursor_position_callback);
    glfwSetKeyCallback(WINDOW, key_callback);

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


    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
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

    glUseProgram(SHADER_PROG1);

    loadTexture();



    while(!glfwWindowShouldClose(WINDOW)) {

        GLuint yoloc = glGetUniformLocation(SHADER_PROG1, "yOffset");
        glUniform1f(yoloc, yOffset);

        glClearColor(0.0f,0.0f,0.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        updateTime();
        castRaysFromCamera();

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, PIXELS);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(WINDOW);
        glfwPollEvents();
    }

    glfwDestroyWindow(WINDOW);
    glfwTerminate();

    return EXIT_SUCCESS;
}