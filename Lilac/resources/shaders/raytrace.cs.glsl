#version 430

layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;


float min3(vec3 v)
{
	return min(min(v.x, v.y), v.z);
}

float max3(vec3 v)
{
	return max(max(v.x, v.y), v.z);
}


// https://tavianator.com/2011/ray_box.html
// see also: https://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms for more research
float intersect_aabb(vec3 aabb_min, vec3 aabb_max, vec3 ray_origin, vec3 ray_inverse_direction)
{
	vec3 t_aabb_min = (aabb_min - ray_origin) * ray_inverse_direction;
	vec3 t_aabb_max = (aabb_max - ray_origin) * ray_inverse_direction;

	vec3 t_min_vec = min(t_aabb_min, t_aabb_max);
	vec3 t_max_vec = max(t_aabb_min, t_aabb_max);

	float t_min = max3(t_min_vec);
	float t_max = min3(t_max_vec);

	return t_min * float(t_max > t_min && t_max > 0);
}


// Simple update
void main() {
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	vec3 pixel_coords_v3 = vec3(pixel_coords.x, pixel_coords.y, 0.0);
	vec3 image_size = vec3(512.0, 512.0, 0.0);
	pixel_coords_v3 = (pixel_coords_v3 - (image_size / 2.0)) / image_size;

	vec3 ray_direction = normalize(vec3(0.0, 0.0, -1.0));
	vec3 ray_up = normalize(vec3(0.0, 1.0, 0.0));
	vec3 ray_right = cross(ray_direction, ray_up);
	vec3 ray_origin = vec3(0.0, 0.0, 2.0) + ray_right * pixel_coords_v3.x + ray_up * pixel_coords_v3.y;

	vec3 aabb_min = vec3(-1.0);
	vec3 aabb_max = vec3(1.0);

	float t = intersect_aabb(
		aabb_min, 
		aabb_max, 
		ray_origin,
		1.0 / ray_direction
	);

	vec4 pixel = vec4(vec3(t), 1.0);

	// output to a specific pixel in the image
	imageStore(img_output, pixel_coords, pixel);
}