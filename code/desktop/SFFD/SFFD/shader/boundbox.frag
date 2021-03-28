#version 460 core

out vec4 color;
uniform vec3 colorInput;

void main()
{
	color	= vec4(colorInput/255, 1.0f);
}