#version 330 core 

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inTex;
layout(location = 2) in vec2 inTex1;
out vec2 chTex;
out vec2 chTex1;

void main()
{
	gl_Position = vec4(inPos.x, inPos.y, 0.0, 1.0);
	chTex = inTex;
	chTex1 = inTex1;
}