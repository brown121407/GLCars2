#ifndef GLCARS2_WINDOW_HPP
#define GLCARS2_WINDOW_HPP

#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include "shader.hpp"

class Window {
public:
    Window(GLuint width, GLuint height, const char* title) {
        if (!glfwInit()) {
            exit(EXIT_FAILURE);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        _window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!_window) {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwMakeContextCurrent(_window);
        glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);

        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW." << std::endl;
            exit(1);
        }

        setup();
        mainLoop();
    }

    ~Window() {
        _shader.reset();

        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
        glDeleteVertexArrays(1, &_triangleVao);
        glDeleteBuffers(1, &_triangleVbo);
        glDeleteBuffers(1, &_colorVbo);

        glfwTerminate();
    }
private:
    GLFWwindow* _window;
    GLuint _triangleVao, _triangleVbo, _colorVbo;
    std::unique_ptr<Shader> _shader;

    void setup() {
        GLfloat vertices[] = {
            -0.5, -0.5, 0.0, 1.0,
            0.0,  0.5, 0.0, 1.0,
            0.5, -0.5, 0.0, 1.0,
        };

        GLfloat colors[] = {
            1.0, 0.0, 0.0, 1.0,
            0.0, 1.0, 0.0, 1.0,
            0.0, 0.0, 1.0, 1.0,
        };

        glGenVertexArrays(1, &_triangleVao);
        glBindVertexArray(_triangleVao);

        glGenBuffers(1, &_triangleVbo);
        glBindBuffer(GL_ARRAY_BUFFER, _triangleVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

        glGenBuffers(1, &_colorVbo);
        glBindBuffer(GL_ARRAY_BUFFER, _colorVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

        _shader = std::make_unique<Shader>("res/shaders/basic.vert", "res/shaders/basic.frag");
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(_window)) {
            processInput(_window);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            render();

            glfwSwapBuffers(_window);
            glfwPollEvents();
        }
    }

    void render() {
        _shader->use();
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glFlush();
    }

    static void processInput(GLFWwindow* window) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    }
};

#endif // GLCARS2_WINDOW_HPP
