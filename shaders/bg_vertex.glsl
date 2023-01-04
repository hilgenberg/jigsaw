out vec2 tex;

void main()
{
	const vec2 q [] = vec2 [] (vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(1, 1));
	vec2 pos = q[gl_VertexID];
	gl_Position = view * vec4((pos.x-0.5)*size.x, (pos.y-0.5)*size.y, 0.0, 1.0);
	tex = vec2(pos.x, pos.y);
}
