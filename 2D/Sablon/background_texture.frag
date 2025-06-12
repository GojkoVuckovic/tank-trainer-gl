#version 330 core

in vec2 chTex;
in vec2 chTex1;
out vec4 outCol;

uniform sampler2D uTex;
uniform sampler2D uTex1;
uniform float uOffsetX;

void main()
{
	vec4 color1 = texture(uTex,vec2(clamp((chTex.s*0.6)+0.2+uOffsetX,0.0,1.0),chTex.t));
	vec4 color2 = texture(uTex1,chTex1);
	outCol = mix(color1,color2,0.4);
}