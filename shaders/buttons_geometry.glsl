#version 430 core

layout(points) in;
in float alpha_geo[];
in uint  index_geo[];

layout(location = 0) uniform sampler2D image;
layout(location = 1) uniform vec2 size; // size of buttons
layout(location = 2) uniform int  n_buttons;

layout(triangle_strip, max_vertices = 4) out;
out vec2 tex;
out flat float alpha;

void main()
{
	uint  i = index_geo[0];
	float a = alpha_geo[0];
	vec4  c = gl_in[0].gl_Position;
	float w = size.x*0.5*a*a, h = size.y*0.5*a*a;
	alpha = a;
	gl_Position = c + vec4(-w, -h, 0.0, 0.0); tex = vec2(0.0, float(i+1)/float(n_buttons)); EmitVertex();
	gl_Position = c + vec4( w, -h, 0.0, 0.0); tex = vec2(1.0, float(i+1)/float(n_buttons)); EmitVertex();
	gl_Position = c + vec4(-w,  h, 0.0, 0.0); tex = vec2(0.0, float(i)  /float(n_buttons)); EmitVertex();
	gl_Position = c + vec4( w,  h, 0.0, 0.0); tex = vec2(1.0, float(i)  /float(n_buttons)); EmitVertex();
	EndPrimitive();
}

