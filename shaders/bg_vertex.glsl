#version 430 core

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform vec2 size; // size of area
layout(location = 2) uniform sampler2D image;
layout(location = 3) uniform float alpha;

out vec2 tex;

void main()
{
	const vec2 q [] = vec2 [] (vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(1, 1));
	vec2 pos = q[gl_VertexID];
	gl_Position = view * vec4((pos.x-0.5)*size.x, (pos.y-0.5)*size.y, 0.0, 1.0);
	tex = vec2(pos.x, pos.y);
}
