#version 330 core
// This code is based on code from here https://learnopengl.com/#!PBR/Lighting
layout (location =0) out vec4 fragColour;

in vec3 worldPos;
in vec3 normal;

// material parameters
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

// lights
uniform vec3 lightPosition;
uniform vec3 lightColor;

uniform vec3 camPos;
uniform float exposure=2.2;

// SSS uniforms
uniform vec3 lightAmbient;
uniform float distortion;
uniform float thickness;
uniform float scale;
uniform float power;
uniform float attenuation;
uniform vec3 diffuseKd;

const float PI = 3.14159265359;

float saturate(float x) {
  return max(0, min(1, x));
}


void main() {
    vec3 lightPos = lightPosition;

    vec3 vNormal = normalize(normal);
    vec3 vEye = normalize(camPos - worldPos);
    vec3 vLight = normalize(worldPos - lightPos);
    vec3 ambient = lightAmbient;

    vec3 vLTLight = vLight + (vNormal * distortion);
    float fLTDot = pow(saturate(dot(vEye, -vLTLight)), power) * scale;
    vec3 fLT = attenuation * (fLTDot * ambient) * thickness;

    vec3 diffuseKd = diffuseKd;
    vec3 lambert = vec3(dot(vEye, vNormal)) * diffuseKd;
    vec3 color = lambert + fLT;
    
    fragColour = vec4(color, 1.0);
}