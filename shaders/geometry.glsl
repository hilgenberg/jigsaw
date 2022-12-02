#version 430 core

layout(points) in;
in  uint borders_geo[];
in  vec2 tex_geo[];

layout(triangle_strip, max_vertices = 4) out;
out vec2 tex_coord;
out flat uint border;
out vec2 orig;

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform sampler2D image;
layout(location = 2) uniform ivec2 count;
layout(location = 3) uniform vec2 size; // size of piece, p.x or p.y will be 1.0, the other greater than

int tx(uint b) { return int(b & uint(1)) - int((b>>1) & uint(1)); }

void main()
{
	uint b  = borders_geo[0];
	int bl = tx(b);
	int br = tx(b>>2);
	int bt = tx(b>>4);
	int bb = tx(b>>6);

	vec4 c = gl_in[0].gl_Position;
	const vec2 q [] = vec2 [] (vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 1.0f));
	vec2 t = tex_geo[0];

	const float R = 1.5f, r = 0.15f, d0 = 0.0f, h = 0.08578643762690485f;
	const float d1 = r+d0, d2 = h;
	float x0 = 0.0f, x1 = 1.0f, y0 = 0.0f, y1 = 1.0f;
	if (bl == 1) x0 -= d1; else if (bl == -1) x0 -= d2;
	if (br == 1) x1 += d1; else if (br == -1) x1 += d2;
	if (bt == 1) y0 -= d1; else if (bt == -1) y0 -= d2;
	if (bb == 1) y1 += d1; else if (bb == -1) y1 += d2;

	border = b;
	orig = vec2(x0, y0);
	gl_Position = view * (c + vec4(x0*size.x, y0*size.y, 0.0f, 0.0f));
	tex_coord   = t + vec2(x0 / count.x, y0 / count.y);
	orig        = vec2(x0, y0);
	EmitVertex();
	gl_Position = view * (c + vec4(x1*size.x, y0*size.y, 0.0f, 0.0f));
	tex_coord   = t + vec2(x1 / count.x, y0 / count.y);
	orig        = vec2(x1, y0);
	EmitVertex();
	gl_Position = view * (c + vec4(x0*size.x, y1*size.y, 0.0f, 0.0f));
	tex_coord   = t + vec2(x0 / count.x, y1 / count.y);
	orig        = vec2(x0, y1);
	EmitVertex();
	gl_Position = view * (c + vec4(x1*size.x, y1*size.y, 0.0f, 0.0f));
	tex_coord   = t + vec2(x1 / count.x, y1 / count.y);
	orig        = vec2(x1, y1);
	EmitVertex();
	EndPrimitive();
}

