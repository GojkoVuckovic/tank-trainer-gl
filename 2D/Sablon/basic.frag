#version 330 core

in vec3 chCol;

out vec4 outCol;

void main(){

	outCol= vec4(chCol,1.0);

}