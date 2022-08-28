#version 330 core

out vec4 frag_color;

in vec2 vert_coord;

uniform sampler2D tex0;
uniform sampler2D tex1;

void main() 
{
	vec4 cr0 = texture(tex0, vert_coord);
	vec4 cr1 = texture(tex1, vert_coord);
	frag_color = mix(cr0, cr1, 0.2);
}
