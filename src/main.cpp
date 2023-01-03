#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>


GLFWwindow* Window;
int ScreenWidth = 640, ScreenHeight = 480;
GLuint ProgramId, VaoId, VboId, ColorBufferId;
GLint ViewLoc, ProjectionLoc;

void Initialize();
void Render();
void Cleanup();
GLuint LoadShaders(const char* vertPath, const char* fragPath);
void CheckCompileErrors(GLuint shader, const std::string& type);

GLuint LoadShaders(const char* vertexSource, const char* fragSource);

int main() {
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make macOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Window = glfwCreateWindow(ScreenWidth, ScreenHeight, "macOS OpenGL", nullptr, nullptr);
    if (!Window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(Window);
    glewInit();
    glfwSwapInterval(1);

    Initialize();

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(Window)) {
        glfwGetFramebufferSize(Window, &ScreenWidth, &ScreenHeight);
        glViewport(0, 0, ScreenWidth, ScreenHeight);

        Render();

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    Cleanup();

    glfwDestroyWindow(Window);
    glfwTerminate();
    return 0;
}

void Initialize() {
    GLfloat vertices[] = {
        -0.5, -0.5, 0.0, 1.0,
        0.0,  0.5, 0.0, 1.0,
        0.5, -0.5, 0.0, 1.0,
    };

    glGenVertexArrays(1, &VaoId);
    glBindVertexArray(VaoId);

    glGenBuffers(1, &VboId);
    glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

    ProgramId = LoadShaders("res/shaders/basic.vert", "res/shaders/basic.frag");
    ViewLoc = glGetUniformLocation(ProgramId, "view");
    ProjectionLoc = glGetUniformLocation(ProgramId, "projection");
}

void Render() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(ProgramId);

    glm::vec3 observator(1.0f, 1.0f, 2.0f);
    glm::vec3 reference(0.0f, 0.0f, -10.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(observator, reference, up);
    GLfloat fovAngle = glm::pi<GLfloat>() / 2.4f; // 75 degrees
    glm::mat4 projection = glm::perspective(fovAngle, (GLfloat) ScreenWidth / (GLfloat) ScreenHeight, 0.1f, 100.0f);

    glUniformMatrix4fv(ViewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(ProjectionLoc, 1, GL_FALSE, &projection[0][0]);

    glBindVertexArray(VaoId);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glFlush();
}

void Cleanup() {
    glDeleteProgram(ProgramId);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &ColorBufferId);
    glDeleteBuffers(1, &VboId);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &VaoId);
}

GLuint LoadShaders(const char *vertPath, const char *fragPath) {
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        // open files
        vShaderFile.open(vertPath);
        fShaderFile.open(fragPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure &e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
    }
    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();
    // 2. compile shaders
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX");
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT");
    // shader Program
    GLuint ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    CheckCompileErrors(ID, "PROGRAM");
    // delete the shaders as they're linked into our program now and no longer necessery
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return ID;
}

void CheckCompileErrors(GLuint shader, const std::string &type) {
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog
                      << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog
                      << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}