layout(location = 0) in vec2  pos;
layout(location = 1) in uint  index;
layout(location = 2) in uint  actv; // "active" is a reserved word

out flat uint index_geo;
out flat uint active_geo;

void main()
{
	gl_Position = vec4(pos, 0.0, 1.0);
	index_geo = index;
	active_geo = actv;
}
