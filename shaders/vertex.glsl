#version 430 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex;
layout(location = 2) in uint borders;

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform sampler2D image;
layout(location = 2) uniform ivec2 count;
layout(location = 3) uniform vec2 size; // size of piece, p.x or p.y will be 1.0, the other greater than

out uint borders_geo;
out vec2 tex_geo;

void main()
{
	gl_Position = vec4(pos, 0.0, 1.0);
	borders_geo = borders;
	tex_geo = tex;
}
