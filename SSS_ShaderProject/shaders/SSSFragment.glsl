#version 330 core
layout (location =0) out vec4 fragColour;

in vec3 worldPos;
in vec3 normal;
in vec2 uv;

// light uniforms
uniform vec3 lightPosition;
uniform vec3 lightColor;

uniform vec3 camPos;

// SSS parameter uniforms are set from imgui -

// ambient value, visible from all angles, represents front & back translucency
uniform vec3 lightAmbient; 
// power value for direct translucency, view dependent
uniform float power;
// subsurface distortion, changes the *surface normal*, view dependent, fresnel-like
uniform float distortion;
// this could be a premade texture map, here just a uniform
uniform float thickness;
// direct/back translucency, view dependent
uniform float scale;
// parameter controlling the falloff of the effecton the whole surface
uniform float attenuation;

float saturate(float x) {
  return max(0, min(1, x));
}

void main()
{
    // frostbite subsurface - from https://colinbarrebrisebois.com/2011/03/07/gdc-2011-approximating-translucency-for-a-fast-cheap-and-convincing-subsurface-scattering-look/
    vec3 lightPos = lightPosition;

    // uses the same names as the presentation for easier translation
    vec3 vNormal = normalize(normal);
    vec3 vEye = normalize(camPos - worldPos);
    vec3 vLight = normalize(worldPos - lightPos);
    vec3 ambient = lightAmbient;

    vec3 vLTLight = vLight + (vNormal * distortion);
    float fLTDot = pow(saturate(dot(vEye, -vLTLight)), power) * scale;
    vec3 fLT = attenuation * (fLTDot * ambient) * thickness;

    vec3 color = fLT;
    fragColour = vec4(fLT, 1.0);
}