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

	// for geo shader: overhang[0] = overhang[1] = h
	float h = 0.034, r = 0.2;
	if (bt*abs(orig.x-0.5) > bt*r && orig.y <     h) discard;
	if (bb*abs(orig.x-0.5) > bb*r && orig.y > 1.0-h) discard;
	if (bl*abs(orig.y-0.5) > bl*r && orig.x <     h) discard;
	if (br*abs(orig.y-0.5) > br*r && orig.x > 1.0-h) discard;

	color = texture(image, tex_coord);
}
