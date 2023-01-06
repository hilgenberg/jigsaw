layout(points) in;
in flat uint index_geo[];
in flat uint active_geo[];

layout(triangle_strip, max_vertices = 4) out;
out vec2 tex;
out flat uint actv; // bool not allowed

void main()
{
	uint  i = index_geo[0];
	vec4  c = gl_in[0].gl_Position;
	float w = size.x*0.5, h = size.y*0.5;
	actv = active_geo[0];
	gl_Position = c + vec4(-w, -h, 0.0, 0.0); tex = vec2(0.0, float(i+1u)/float(n_buttons)); EmitVertex();
	gl_Position = c + vec4( w, -h, 0.0, 0.0); tex = vec2(1.0, float(i+1u)/float(n_buttons)); EmitVertex();
	gl_Position = c + vec4(-w,  h, 0.0, 0.0); tex = vec2(0.0, float(i)   /float(n_buttons)); EmitVertex();
	gl_Position = c + vec4( w,  h, 0.0, 0.0); tex = vec2(1.0, float(i)   /float(n_buttons)); EmitVertex();
	EndPrimitive();
}

