in  vec2      tex_coord;
in  flat uint border;
in  vec2      orig;
out vec4      color;

float tx(uint b) { return float(b & uint(1))-float((b>>1) & uint(1)); }

void main()
{
	float bl = tx(border); // +1 if knob comes out, -1 if knob goes in
	float br = tx(border>>2);
	float bt = tx(border>>4);
	float bb = tx(border>>6);

	// h1 is the distance of the tip to the original edge
	// h2 is the inset on the other side
	// const float h1 = overhang[0], h2 = overhang[1], w = 0.5f*h2/h1;
	const float h1 = 0.15, h2 = 0.05, w = 0.5/3.0;

	float x = abs(orig.x-0.5), y = abs(orig.y-0.5);
	float a = (h1+h2)/w, b = h1*h2/(h1-h2);
	float xx = min(a*x - h1, b - 2.0*b*x);
	float yy = min(a*y - h1, b - 2.0*b*y);

	if (    orig.y < bt * xx) discard;
	if (1.0-orig.y < bb * xx) discard;
	if (    orig.x < bl * yy) discard;
	if (1.0-orig.x < br * yy) discard;

	color = texture(image, tex_coord);
}
