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

uniform float wrap;
uniform float scatterWidth;
uniform vec4 scatterColour;
uniform float shininess;


void main () {
    
    vec3 N = normalize(fragmentNormal);
    vec3 L = normalize(light.position);
    //vec3 diff = vec3(dot(N,L));
    //fragColour = diffuse *light.diffuse *vec4(diff,1.0);
    

    //float NdotL = P.x * 2 - 1;  // remap from [0, 1] to [-1, 1] ---- where do I define Position? should I use inVert?
    float NdotL = dot(N,L);
    
    //float NdotH = P.y * 2 - 1; // where is H defined?

    float NdotL_wrap = (NdotL + wrap) / (1 + wrap); // wrap lighting

    float diffuse = max(NdotL_wrap, 0.0); // I have diffuse defined a few times idk which one is right

    // add color tint at transition from light to dark
  
    float scatter = smoothstep(0.0, scatterWidth, NdotL_wrap) * smoothstep(scatterWidth * 2.0, scatterWidth, NdotL_wrap);

    //float specular = pow(NdotH, shininess);

    //if (NdotL_wrap <= 0) specular = 0;
    //    vec4 C;
    //    C.rgb = diffuse + scatter * scatterColour;
    //    C.a = specular;
    //    return C;
    

    // Shade using lookup table

    //vec3 ShadeSkin(sampler2D skinLUT, vec3 N, vec3 L, vec3 H, vec3 diffuseColor, vec3 specularColor) : COLOUR
    //{
    //vec2 s;
    //s.x = dot(N, L);
    //s.y = dot(N, H);
    //vec4 light = tex2D(skinLUT, s * 0.5 + 0.5);
    //return diffuseColor * light.rgb + specularColour * light.a;

    fragColour = diffuse + scatter * scatterColour;

    



    //float gemdiffuse = max(0, dot(L, N));
    //float gemwrap_diffuse = max(0, (dpt(L, N) + wrap)/ (1 + wrap));


}

