#version 330 core

out vec3 fragmentNormal;
out vec3 fragmentPosition;

layout (location=0) in vec3 inVert;
layout (location=1) in vec3 inNormal;

uniform mat4 MV;
uniform vec3 lightPos;
uniform mat4 MVP;
uniform mat3 normalMatrix;

void main() {
	fragmentNormal = (normalMatrix*inNormal);
	fragmentPosition = inVert;
	gl_Position = MVP*vec4(inVert,1.0);
}







