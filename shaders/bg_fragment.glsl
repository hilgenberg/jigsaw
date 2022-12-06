#version 430 core

layout(location = 0) uniform mat4 view;

out vec4 color;

float tx(uint b) { return float(b & uint(1))-float((b>>1) & uint(1)); }
float dq(vec2 v, float r) { return dot(v,v) - r*r; }

void main()
{
	color = vec4(0.3, 0.3, 0.3, 1.0);
}
