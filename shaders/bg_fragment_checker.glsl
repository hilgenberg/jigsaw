in  vec2 tex;
out vec4 color;

void main()
{
	vec4 c [] = vec4 [] (bg_light, bg_dark);
	int i = int(floor(tex.x*size.x + view[3].x*0.3/view[0].x));
	int j = int(floor(tex.y*size.y + view[3].y*0.3/view[1].y));

	color = c[(i+j)&1];
}
