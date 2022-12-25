#version 430
layout(std430, binding = 0) buffer Pixels
{
	vec4 colors[]; // Should match compute shader
};
const ivec2 image_size = ivec2(512, 512);
const ivec3 work_group_size = ivec3(16, 16, 1);
const uint work_group_total_size = work_group_size.x * work_group_size.y * work_group_size.z;

in vec2 v_tex_coord;
out vec4 frag_color;

void main() 
{
	// vec2 frag_coord = floor(gl_FragCoord.xy * image_size / vec2(512, 512));
	vec2 frag_coord = floor((image_size - ivec2(1)) * v_tex_coord); // Subtract 1 because we go from 0..1 inclusive
	ivec2 frag_coord_ivec = ivec2(frag_coord);
	
	uint index = work_group_total_size * (frag_coord_ivec.x + frag_coord_ivec.y * image_size.x);
	frag_color = vec4(0.0, 0.0, 0.0, 1.0);

	// frag_color.r = index / float(image_size.x * image_size.y);
	// frag_color.rg = frag_coord.xy / vec2(image_size);

	// frag_color = colors[index];

	float weight = 1.0 / float(work_group_total_size);
	for (int i = 0; i < work_group_total_size; i++)
	{
		frag_color += weight * colors[index + i];
	}
}