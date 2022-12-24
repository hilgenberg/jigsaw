#version 430 core

layout(points) in;
in uint borders_geo[];
in vec2 tex_geo[];

layout(triangle_strip, max_vertices = 4) out;
out vec2 tex_coord;
out flat uint border;
out vec2 orig;

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform sampler2D image;
layout(location = 2) uniform ivec2 count;
layout(location = 3) uniform vec2 size; // size of piece, p.x or p.y will be 1.0, the other one greater
layout(location = 4) uniform vec2 overhang; // how far do the pieces stick out from the border? (out_border,in_border)

int tx(uint b) { return int(b & uint(1)) - int((b>>1) & uint(1)); }

void main()
{
	uint b  = borders_geo[0];
	int bl = tx(b);
	int br = tx(b>>2);
	int bt = tx(b>>4);
	int bb = tx(b>>6);

	vec4 c = gl_in[0].gl_Position;
	vec2 t = tex_geo[0];

	float x0 = 0.0f, x1 = 1.0f, y0 = 0.0f, y1 = 1.0f;
	if (bl == 1) x0 -= overhang[0]; else if (bl == -1) x0 -= overhang[1];
	if (br == 1) x1 += overhang[0]; else if (br == -1) x1 += overhang[1];
	if (bt == 1) y0 -= overhang[0]; else if (bt == -1) y0 -= overhang[1];
	if (bb == 1) y1 += overhang[0]; else if (bb == -1) y1 += overhang[1];

	border = b;
	
	gl_Position = view * (c + vec4(x0*size.x, y0*size.y, 0.0, 0.0));
	tex_coord   = t + vec2(x0 / count.x, y0 / count.y);
	orig        = vec2(x0, y0); // position inside the piece (could be computed from tex_coord, but messy)
	EmitVertex();

	gl_Position = view * (c + vec4(x1*size.x, y0*size.y, 0.0, 0.0));
	tex_coord   = t + vec2(x1 / count.x, y0 / count.y);
	orig        = vec2(x1, y0);
	EmitVertex();
	
	gl_Position = view * (c + vec4(x0*size.x, y1*size.y, 0.0, 0.0));
	tex_coord   = t + vec2(x0 / count.x, y1 / count.y);
	orig        = vec2(x0, y1);
	EmitVertex();
	
	gl_Position = view * (c + vec4(x1*size.x, y1*size.y, 0.0, 0.0));
	tex_coord   = t + vec2(x1 / count.x, y1 / count.y);
	orig        = vec2(x1, y1);
	EmitVertex();
	
	EndPrimitive();
}

