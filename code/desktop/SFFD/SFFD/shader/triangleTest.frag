#version 330 core


in vec4 geo_color;
out vec4 color;

void main()
{
	color= geo_color;
	//color=vec4(0,0,0,1);
}