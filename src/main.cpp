#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector> 
#include <cstdlib>
#include <array>

#define TEXT_LOADER_IMPLEMENTATION
#include "textloader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


int forward = 0;
int backward = 0;
int lookleft = 0;
int lookright = 0;

GLFWwindow * WINDOW;

GLuint SHADER_PROG1;
GLuint TEXTURE_ID;


float VIEWDISTANCE = 7.0f;

const int WIDTH = 1280;
const int HEIGHT = 720;

const int NUMCHANNELS = 3;

float lastFrame = 0.0f;
float deltaTime = 0.0f;

float yOffset = 0.0f;

void updateTime() {
    double currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
}


GLubyte PIXELS[WIDTH * HEIGHT * NUMCHANNELS];

GLuint stoneTexture = 0;
int nrChannels;
GLubyte* stoneWallTexture;

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
}

int MAPWIDTH = 5;

int MAP[] = {
    1, 1, 1, 1, 1,
    1, 0, 1, 0, 1,
    0, 0, 0, 0, 0,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1
};

glm::ivec3 colorFromUV(float uvX, float uvY, GLubyte* texture) {
    //assuming texture is always 32x32

    int x = std::round(uvX * 32.0f);
    int y = std::round((1.0f - uvY )* 31.0f);

    int index = y * 32 + x;

    int realIndex = index * nrChannels;

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

void castRaysFromCamera() {

    if(lookright) {
        
        cameraPosition += directionFromAngle(cameraAngle+((std::acos(-1.0f)/2.0f)))*deltaTime;
    }
    if(lookleft) {
        cameraPosition += directionFromAngle(cameraAngle-((std::acos(-1.0f)/2.0f)))*deltaTime;
    }
    if(forward) {
        cameraPosition += directionFromAngle(cameraAngle)*deltaTime;
    }
    if(backward) {
        cameraPosition -= directionFromAngle(cameraAngle)*deltaTime;
    }


    for(int col = 0; col < WIDTH; col++) {

        float angle = ((((col - (WIDTH / 2)) / (WIDTH/2.0f) + 1.0f) * std::acos(-1.0f)) / 4.0f) - std::acos(-1.0f)/4.0f;

        //std::cout << angle << "\n";
        glm::vec2 rayDir = directionFromAngle(angle + cameraAngle);
        //std::cout << rayDir.x << " " << rayDir.y << "\n";

        float travel = 0;

        int hit = -1;

        glm::vec2 testSpot;

        for(float i = 0; i < VIEWDISTANCE; i += 0.01f) {
            testSpot = cameraPosition + (rayDir * i);
            int sampled = sampleMap(std::floor(testSpot.x), std::floor(testSpot.y));
            
            hit = sampled;
            travel+=0.01f;
            if(sampled == 1 || sampled == -1) {

                
                if(sampled == 1) {
                    float rolledBack = 0.0f;
                    //std::cout << "travel was " << travel << "\n";
                    while (sampleMap(std::floor(testSpot.x), std::floor(testSpot.y)) != 0 && sampleMap(std::floor(testSpot.x), std::floor(testSpot.y)) != -1) {
                        testSpot -= rayDir*0.001f;
                        travel -= 0.001f;
                        rolledBack += 0.001f;
                    }

                    testSpot += rayDir*0.005f;
                    //std::cout << "rolled back travel " << rolledBack << " to " << travel << "\n";
                }
                


                break;
            }
        }



        

        if(hit == 0 || hit == -1) {
            for(int i = 0; i < HEIGHT; i++) {
                int ind = pixelIndexFromCoord(col, i);
                if(ind != -1) {
                    setPixel(ind, 0, 0, 0);
                }
            }
        } else {

            float zmod = std::fmod(testSpot.y, 1.0f);
            float xmod = std::fmod(testSpot.x, 1.0f);

            float uvX;

            if(std::abs(zmod) > std::abs(xmod)) {
                uvX = std::abs(zmod);
            } else {
                uvX = std::abs(xmod);
            }


            //std::cout << "hit on col " << col << " with travel " << travel << "\n";'

            int height = std::max(5.0f - (travel), 1.5f) * 50;

            int trav = 0;
            for(int i = HEIGHT/2; i < HEIGHT; i++) {
                int ind = pixelIndexFromCoord(col, i);
                if(ind != -1) {
                    float uvY = 0.5f - (((float)trav/height) / 2.0f);
                    if(trav < height) {
                        

                        glm::ivec3 color = colorFromUV(uvX, uvY, stoneWallTexture);
                        setPixel(ind, std::max(0, color.r - (int)(travel*20)), std::max(0, color.g - (int)(travel*20)), std::max(0, color.b - (int)(travel*20)));
                    } else {
                        setPixel(ind, 0, 0, 0);
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
                        glm::ivec3 color = colorFromUV(uvX, uvY, stoneWallTexture);
                        setPixel(ind, std::max(0, color.r - (int)(travel*20)), std::max(0, color.g - (int)(travel*20)), std::max(0, color.b - (int)(travel*20)));
                    } else {
                        setPixel(ind, 0, 0, 0);
                    }
                    
                }
                trav++;
            }
        }
    }

}

bool mouseCaptured = false;



void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
    }
        

}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    static double lastX = 0;
    static double lastY = 0;

    static bool firstMouse = true;
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

        yOffset = std::max(std::min(yOffset + yDelta, 0.5), -0.5);
        
    }
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_A)
    {
        lookleft = action;
        
    }
    if (key == GLFW_KEY_D)
    {
        lookright = action;
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
    if (!(WINDOW = glfwCreateWindow(WIDTH, HEIGHT, "RayTest", NULL, NULL))) {
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
    -1.0f,  1.0f,  0.0f, 0.0f, // Top Left
    -1.0f, -1.0f,  0.0f, 1.0f, // Bottom Left
     1.0f, -1.0f,  1.0f, 1.0f, // Bottom Right

    -1.0f,  1.0f,  0.0f, 0.0f, // Top Left
     1.0f, -1.0f,  1.0f, 1.0f, // Bottom Right
     1.0f,  1.0f,  1.0f, 0.0f  // Top Right
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