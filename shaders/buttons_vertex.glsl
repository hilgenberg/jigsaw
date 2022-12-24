#version 430 core

layout(location = 0) in vec2 pos;
layout(location = 1) in float alpha;
layout(location = 2) in uint index;

layout(location = 0) uniform sampler2D image;
layout(location = 1) uniform vec2 size; // size of buttons
layout(location = 2) uniform int  n_buttons;

out float alpha_geo;
out uint  index_geo;

void main()
{
	gl_Position = vec4(pos, 0.0, 1.0);
	alpha_geo = alpha;
	index_geo = index;
}
