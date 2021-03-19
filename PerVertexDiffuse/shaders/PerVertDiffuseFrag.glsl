#version 330 core

in vec3 fragmentNormal;
in vec3 fragmentPosition;
layout (location =0)out vec4 fragColour;



struct Light
{
    vec3 position;
    vec4 diffuse;
};

uniform Light light;

uniform vec4 diffuse;

void main () {
    
    vec3 N = normalize(fragmentNormal);
    vec3 L = normalize(light.position);
    vec3 diff = vec3(dot(N,L));
    fragColour = diffuse *light.diffuse *vec4(diff,1.0);
}

