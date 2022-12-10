#version 430 core

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform sampler2D image;

in  vec2      tex_coord;
in  flat uint border;
in  vec2      orig;
out vec4      color;

float tx(uint b) { return float(b & uint(1))-float((b>>1) & uint(1)); }

void main()
{
	const float bl = tx(border); // +1 if knob comes out, -1 if knob goes in
	const float br = tx(border>>2);
	const float bt = tx(border>>4);
	const float bb = tx(border>>6);

	// h1 is the distance of the tip to the original edge
	// h2 is the inset on the other side
	// const float h1 = overhang[0], h2 = overhang[1], w = 0.5f*h2/h1;
	const float h1 = 0.15, h2 = 0.05, w = 0.5/3.0;

	const float x = abs(orig.x-0.5), y = abs(orig.y-0.5);
	const float a = (h1+h2)/w, b = h1*h2/(h1-h2);
	const float xx = min(a*x - h1, b - 2.0*b*x);
	const float yy = min(a*y - h1, b - 2.0*b*y);

	if (    orig.y < bt * xx) discard;
	if (1.0-orig.y < bb * xx) discard;
	if (    orig.x < bl * yy) discard;
	if (1.0-orig.x < br * yy) discard;

	color = texture(image, tex_coord);
}
