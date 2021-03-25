#version 410 core

//basic vertex shader based on Jon's Simple NGL demo

/// @brief the vertex passed in
layout (location = 0) in vec3 inVert;
/// @brief the normal passed in
layout (location = 1) in vec3 inNormal;
/// @brief the in uv
layout (location = 2) in vec2 inUV;

out vec3 worldPos;
out vec3 normal;
out vec2 uv;

layout( std140) uniform TransformUBO
{
  mat4 MVP;
  mat4 normalMatrix;
  mat4 M;
} transforms;

void main()
{
	// transform the vertex from object space to world (but do not apply the camera or projection transform)
	worldPos = vec3(transforms.M * vec4(inVert, 1.0f));

	// transform the normal too
	normal = normalize(mat3(transforms.normalMatrix) * inNormal);

	// the final vertex position drawn to screen
	gl_Position = transforms.MVP*vec4(inVert,1.0);

	// TODO use these to read a thickness texture
	uv = inUV;
}
