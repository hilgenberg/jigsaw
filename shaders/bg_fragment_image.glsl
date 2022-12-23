#version 430 core

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform vec2 size; // size of area
layout(location = 2) uniform sampler2D image;
layout(location = 3) uniform float alpha;

in  vec2 tex;
out vec4 color;

void main()
{
	color = alpha*texture(image, tex) + (1.0-alpha)*vec4(0.3, 0.3, 0.3, 1.0);
}
