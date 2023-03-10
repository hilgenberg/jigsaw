#define M_PI 3.1415926535897932384626433832795

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

	const float r = 0.2;
	float x = abs(orig.x-0.5), y = abs(orig.y-0.5);

	if (bt*orig.y < 0.0 && bt*length(vec2(x,     orig.y)) > bt*r) discard;
	if (bb*orig.y > bb  && bb*length(vec2(x, 1.0-orig.y)) > bb*r) discard;
	if (bl*orig.x < 0.0 && bl*length(vec2(y,     orig.x)) > bl*r) discard;
	if (br*orig.x > br  && br*length(vec2(y, 1.0-orig.x)) > br*r) discard;

	color = texture(image, tex_coord);
}
