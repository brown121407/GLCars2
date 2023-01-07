#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <utility>
#include <memory>
#include <vector>
#include <map>

GLFWwindow* Window;
int ScreenWidth = 640, ScreenHeight = 480;
GLuint ProgramId;
GLint ModelLoc, ViewLoc, ProjectionLoc;
GLint MaterialAmbientLoc, MaterialDiffuseLoc, MaterialSpecularLoc, MaterialShininessLoc, MaterialOpacityLoc;
GLint LightPositionLoc, LightAmbientLoc, LightDiffuseLoc, LightSpecularLoc;

void Initialize();
void Render();
void Cleanup();
GLuint LoadShaders(const char* vertPath, const char* fragPath);
void CheckCompileErrors(GLuint shader, const std::string& type);

class Model;
std::unique_ptr<Model> Car;


struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Material {
    glm::vec3 Ambient;
    glm::vec3 Diffuse;
    glm::vec3 Specular;
    float Shininess;
    float Opacity;
};

class Mesh {
public:
    Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, Material material)
    : _vertices(std::move(vertices)), _indices(std::move(indices)), _material(material) {
        glGenVertexArrays(1, &_vaoId);
        glBindVertexArray(_vaoId);

        glGenBuffers(1, &_vboId);
        glBindBuffer(GL_ARRAY_BUFFER, _vboId);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * _vertices.size(), &_vertices[0], GL_STATIC_DRAW);

        glGenBuffers(1, &_eboId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _eboId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * _indices.size(), &_indices[0], GL_STATIC_DRAW);

        // Vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0);

        // Normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, Normal));
    }

    Mesh(Mesh&& other) noexcept
    : _vaoId(other._vaoId), _vboId(other._vboId), _eboId(other._eboId), _vertices(std::move(other._vertices)),
    _indices(std::move(other._indices)), _material(other._material) {

    }

    ~Mesh() {
        glDeleteVertexArrays(1, &_vaoId);
        glDeleteBuffers(1, &_vboId);
        glDeleteBuffers(1, &_eboId);
    }

    void Render() const {
        glBindVertexArray(_vaoId);

        glUniform3fv(MaterialAmbientLoc, 1, &_material.Ambient[0]);
        glUniform3fv(MaterialDiffuseLoc, 1, &_material.Diffuse[0]);
        glUniform3fv(MaterialSpecularLoc, 1, &_material.Specular[0]);
        glUniform1f(MaterialShininessLoc, _material.Shininess);
        glUniform1f(MaterialOpacityLoc, _material.Opacity);
        glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_INT, nullptr);

        glBindVertexArray(0);
    }
private:
    std::vector<Vertex> _vertices;
    std::vector<GLuint> _indices;
    Material _material;
    GLuint _vaoId, _vboId, _eboId;
};

class Model {
public:
    explicit Model(const char* path) {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            exit(1);
        }

        processNode(scene->mRootNode, scene);
    }

    void Render() const {
        for (const auto& [_, mesh] : _meshes) {
            mesh->Render();
        }
    }

    void Render(const std::function<bool(const std::string&)>& namePredicate) const {
        for (const auto& [name, mesh] : _meshes) {
            if (namePredicate(name)) {
                mesh->Render();
            }
        }
    }
private:
    std::map<std::string, std::unique_ptr<Mesh>> _meshes;

    void processNode(aiNode* node, const aiScene* scene) {
        for (size_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            _meshes[node->mName.C_Str()] = processMesh(mesh, scene);
        }

        for (size_t i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    std::unique_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        for (int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex{};
            vertex.Position.x = mesh->mVertices[i].x;
            vertex.Position.y = mesh->mVertices[i].y;
            vertex.Position.z = mesh->mVertices[i].z;

            if (mesh->HasNormals()) {
                vertex.Normal.x = mesh->mNormals[i].x;
                vertex.Normal.y = mesh->mNormals[i].y;
                vertex.Normal.z = mesh->mNormals[i].z;
            }
            vertices.push_back(vertex);
        }

        std::vector<GLuint> indices;
        for (int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiColor4D ambient, diffuse, specular;
        GLfloat opacity, shininess;
        material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
        material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        material->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
        material->Get(AI_MATKEY_OPACITY, opacity);
        material->Get(AI_MATKEY_SHININESS, shininess);

        Material mat {
            .Ambient = glm::vec3(ambient.r, ambient.g, ambient.b),
            .Diffuse = glm::vec3(diffuse.r, diffuse.g, diffuse.b),
            .Specular = glm::vec3(specular.r, specular.g, specular.b),
            .Shininess = shininess,
            .Opacity = opacity,
        };

        return std::make_unique<Mesh>(vertices, indices, mat);
    }
};

int main() {
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make macOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Window = glfwCreateWindow(ScreenWidth, ScreenHeight, "GLCars2", nullptr, nullptr);
    if (!Window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(Window);
    glewInit();
    glfwSwapInterval(1);

    Initialize();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
    Car = std::make_unique<Model>("res/models/car/car.obj");

    ProgramId = LoadShaders("res/shaders/basic.vert", "res/shaders/basic.frag");
    ModelLoc = glGetUniformLocation(ProgramId, "model");
    ViewLoc = glGetUniformLocation(ProgramId, "view");
    ProjectionLoc = glGetUniformLocation(ProgramId, "projection");
    MaterialAmbientLoc = glGetUniformLocation(ProgramId, "material.ambient");
    MaterialDiffuseLoc = glGetUniformLocation(ProgramId, "material.diffuse");
    MaterialSpecularLoc = glGetUniformLocation(ProgramId, "material.specular");
    MaterialOpacityLoc = glGetUniformLocation(ProgramId, "material.opacity");

    LightPositionLoc = glGetUniformLocation(ProgramId, "light.position");
    LightAmbientLoc = glGetUniformLocation(ProgramId, "light.ambient");
    LightDiffuseLoc = glGetUniformLocation(ProgramId, "light.diffuse");
    LightSpecularLoc = glGetUniformLocation(ProgramId, "light.specular");
}

void Render() {
    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(ProgramId);

    glm::vec3 reference(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    float radius = 10.0f;
    float camX = glm::sin(glfwGetTime() / 2) * radius;
    float camZ = glm::cos(glfwGetTime() / 2) * radius;
    glm::vec3 cameraPos = glm::vec3(camX, 5.0f, camZ);
    glm::mat4 view = glm::lookAt(cameraPos, reference, up);

    glm::mat4 projection = glm::perspective(glm::radians(75.0f), (GLfloat) ScreenWidth / (GLfloat) ScreenHeight, 0.1f, 100.0f);

    glUniformMatrix4fv(ViewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(ProjectionLoc, 1, GL_FALSE, &projection[0][0]);

    auto model = glm::mat4(1.0f);
    glUniformMatrix4fv(ModelLoc, 1, GL_FALSE, &model[0][0]);

    glm::vec3 lightPosition(10.2f, 15.0f, -10.0f);
    glUniform3fv(LightPositionLoc, 1, &lightPosition[0]);

    glm::vec3 lightAmbient(0.1f);
    glm::vec3 lightDiffuse(1.0f);
    glm::vec3 lightSpecular(1.0f);
    glUniform3fv(LightAmbientLoc, 1, &lightAmbient[0]);
    glUniform3fv(LightDiffuseLoc, 1, &lightDiffuse[0]);
    glUniform3fv(LightSpecularLoc, 1, &lightSpecular[0]);

    Car->Render();

    glFlush();
}

void Cleanup() {
    glDeleteProgram(ProgramId);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
}

GLuint LoadShaders(const char *vertPath, const char *fragPath) {
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream models can throw exceptions:
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