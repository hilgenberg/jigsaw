#version 430 core

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform vec2 size; // size of area

in  vec2 tex;
out vec4 color;

void main()
{
	const vec3 c [] = vec3 [] (vec3(0.3, 0.3, 0.3), vec3(0.29, 0.29, 0.29));
	int i = int(floor(tex.x*size.x + view[3].x*0.3f/view[0].x));
	int j = int(floor(tex.y*size.y + view[3].y*0.3f/view[1].y));

	color = vec4(c[(i+j)&1], 1.0);
}
