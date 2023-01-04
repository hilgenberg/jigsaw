in  vec2 tex;
out vec4 color;

void main()
{
	color = alpha*texture(image, tex) + (1.0-alpha)*vec4(0.3, 0.3, 0.3, 1.0);
}
