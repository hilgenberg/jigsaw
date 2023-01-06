layout(location = 0) in vec2 pos; // of top-left corner or the piece
layout(location = 1) in vec2 tex; // of top-left corner
layout(location = 2) in uint borders; // see fragment shaders for how this is decoded

out flat uint borders_geo;
out vec2 tex_geo;

void main()
{
	gl_Position = vec4(pos, 0.0, 1.0);
	borders_geo = borders;
	tex_geo = tex;
}
