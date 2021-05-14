#version 330 core

out vec4 color;
uniform int pixel;

void main()
{

	if (pixel == 1)
		color = vec4(1.0f,1.0f,1.0f,1.0f);
	else if (pixel == 0)
		color = vec4(0.0f,0.0f,0.0f,1.0f);
	else 
		color = vec4(0.6f,0.6f,0.6f,1.0f);
}