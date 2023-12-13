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

#include <opencv2/opencv.hpp>


GLFWwindow * WINDOW;
int WWIDTH = 1280;
int WHEIGHT = 720;

GLuint SHADER_PROG1;
GLuint TEXTURE_ID;

//Uncomment this stuff to remove console when done:
//#include <Windows.h>
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
int main() {

    cv::VideoCapture cap(0); //Open el default camero

    if (!cap.isOpened()) {
        std::cerr << "Error opening video stream or file" << std::endl;
        return EXIT_FAILURE;
    }



    cv::Mat frame;
    cap >> frame;  //Take an initial cap to get the size;







    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    if (!(WINDOW = glfwCreateWindow(frame.cols, frame.rows, "CameraTest", NULL, NULL))) {
        glfwTerminate();
        return EXIT_FAILURE;
    }


    glfwMakeContextCurrent(WINDOW);
    glewInit();

    //glfwSetMouseButtonCallback(WINDOW, mouse_button_callback);
    //glfwSetCursorPosCallback(WINDOW, cursor_position_callback);
    //glfwSetKeyCallback(WINDOW, key_callback);

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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame.cols, frame.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

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

    double updateSimTimer = 0.0;

    while(!glfwWindowShouldClose(WINDOW)) {

        glClearColor(0.0f,0.0f,0.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        cap >> frame;
        if(frame.empty()) {
            break;
        }
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, frame.rows, GL_RGB, GL_UNSIGNED_BYTE, frame.data);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(WINDOW);
        glfwPollEvents();
    }

    cap.release();
    glfwDestroyWindow(WINDOW);
    glfwTerminate();

    return EXIT_SUCCESS;
}