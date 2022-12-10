#version 430 core

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform vec2 size; // size of area

out vec2 tex;

void main()
{
	const vec2 q [] = vec2 [] (vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(-0.5, 0.5), vec2(0.5, 0.5));
	vec2 pos = q[gl_VertexID];
	gl_Position = view * vec4(pos.x*size.x, pos.y*size.y, 0.0, 1.0);
	tex = vec2(pos.x*size.x, pos.y*size.y);
}
