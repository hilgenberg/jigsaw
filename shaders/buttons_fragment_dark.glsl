in  vec2  tex;
in  flat uint actv;
out vec4  color;

void main()
{
	color = texture(image, tex);
	color.r = 1.0 - color.r;
	color.g = 1.0 - color.g;
	color.b = 1.0 - color.b;
	if (actv == 0u) color.a *= 0.25;
}
