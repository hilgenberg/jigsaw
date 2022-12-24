#version 430 core

layout(location = 0) uniform sampler2D image;
layout(location = 1) uniform vec2 size; // size of buttons
layout(location = 2) uniform int  n_buttons;

in  vec2  tex;
in  flat float alpha;
out vec4  color;

void main()
{
	color = texture(image, tex);
	color.a *= alpha;
}
