#version 430 core

//layout(location = 0) in vec2 pos;
layout(location = 0) uniform mat4 view;
layout(location = 1) uniform vec2 size; // size of area

void main()
{
	const vec2 q [] = vec2 [] (vec2(-1.0f, -1.0f), vec2(1.0f, -1.0f), vec2(-1.0f, 1.0f), vec2(1.0f, 1.0f));
	vec2 pos = q[gl_VertexID];
	gl_Position = view * vec4(pos.x*size.x, pos.y*size.y, 0.0, 1.0);
}
