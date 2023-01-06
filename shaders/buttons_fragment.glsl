in  vec2  tex;
in  flat uint actv;
out vec4  color;

void main()
{
	color = texture(image, tex);
	if (actv == 0u) color *= 0.5;
}
