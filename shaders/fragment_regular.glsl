#version 430 core

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform sampler2D image;
layout(location = 2) uniform ivec2 count;
layout(location = 3) uniform vec2 size; // size of piece, p.x or p.y will be 1.0, the other one gte

in  vec2      tex_coord;
in  flat uint border;
in  vec2      orig;
out vec4      color;

float tx(uint b) { return float(b & uint(1))-float((b>>1) & uint(1)); }
float dq(vec2 v, float r) { return dot(v,v) - r*r; }

void main()
{
	const float bl = tx(border); // +1 if knob comes out, -1 if knob goes in
	const float br = tx(border>>2);
	const float bt = tx(border>>4);
	const float bb = tx(border>>6);

	// R is radius of big circle
	// r is radius of small circle
	// d0 is the distance of the small circle's center to the original edge
	// h is computed from R (height of the area that the big circle cuts off)
	// for geo shader: overhang[0] = r+d0, overhang[1] = h
	const float R = 1.5, r = 0.15, d0 = 0.0, h = 0.08578643762690485;
	if (bl*max(dq(vec2(    bl*(h-R), 0.5)-orig.xy, R), -dq(vec2(   -bl*d0, 0.5)-orig.xy, r)) < -1e-8) discard;
	if (br*max(dq(vec2(1.0+br*(R-h), 0.5)-orig.xy, R), -dq(vec2(1.0+br*d0, 0.5)-orig.xy, r)) < -1e-8) discard;
	if (bt*max(dq(vec2(0.5,     bt*(h-R))-orig.xy, R), -dq(vec2(0.5,    -bt*d0)-orig.xy, r)) < -1e-8) discard;
	if (bb*max(dq(vec2(0.5, 1.0+bb*(R-h))-orig.xy, R), -dq(vec2(0.5, 1.0+bb*d0)-orig.xy, r)) < -1e-8) discard;
	color = texture(image, tex_coord);
}
