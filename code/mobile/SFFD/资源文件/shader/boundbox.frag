#version 310 es

precision mediump float;
out vec4 color;
uniform vec3 colorInput;

void main()
{
	color	= vec4(colorInput/255.0f, 1.0f);
}