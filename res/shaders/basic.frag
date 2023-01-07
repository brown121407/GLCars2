#version 330 core

in vec3 Normal;
out vec4 FragColor;

uniform int colorCode = 0;

void main() {
    switch (colorCode) {
        case 1:
            FragColor = vec4(0.41);
            break;
        case 2:
            FragColor = vec4(0.2);
            break;
        default:
            FragColor = vec4(0.1, 1.0, 0.1, 1.0);
            break;
    }
}