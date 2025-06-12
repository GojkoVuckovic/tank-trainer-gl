#version 330 core

out vec4 outCol;

uniform vec3 uObjectColor;

void main()
{
	outCol = vec4(uObjectColor, 1.0f);
}