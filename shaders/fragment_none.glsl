in  vec2 tex_coord;
in  flat uint border; // unused here
in  vec2 orig; // this too
out vec4 color;

void main()
{
	color = texture(image, tex_coord);
}
