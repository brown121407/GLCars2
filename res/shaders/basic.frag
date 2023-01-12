#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emission;
    float shininess;
    float opacity;
};

struct Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform Light light;
uniform vec3 viewPos;

in vec3 FragPos;
in vec3 Normal;
out vec4 FragColor;

void main() {
    vec3 ambient = light.ambient * material.ambient;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * (diff * material.diffuse);

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * (spec * material.specular);

    vec3 result = material.emission + ambient + diffuse;

    vec3 fogColor = vec3(0.5, 0.6, 0.7);
    float distToFrag = distance(vec3(0.0), FragPos);
    float fullFogThreshold = 50.0;
    float fogFactor = clamp((fullFogThreshold - distToFrag) / distToFrag, 0, 1);
    vec3 finalColor = fogFactor * result + (1 - fogFactor) * fogColor;

    FragColor = vec4(finalColor, material.opacity);
}