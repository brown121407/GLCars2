#version 330 core

layout (location = 0) in vec4 aPos;

uniform mat4 model, view, projection;

void main() {
    gl_Position = projection * view * model * aPos;
}