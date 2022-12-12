#version 430

layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;


const vec3 inf3 = vec3(1.0) / vec3(0.0);
const vec3 neg_inf3 = vec3(-1.0) / vec3(0.0);

const float inf = 1.0 / 0.0;
const float neg_inf = -1.0 / 0.0;


float min3(vec3 v)
{
	return min(min(v.x, v.y), v.z);
}

float max3(vec3 v)
{
	return max(max(v.x, v.y), v.z);
}


// https://tavianator.com/2011/ray_box.html
// The weird seeming min(max(x, neg_inf)) stuff is to prevent the issue
// where 0.0 * inf = NaN, and NaN's are propigated based on the first argument 
// in standard implementations
// see: https://tavianator.com/2022/ray_box_boundary.html
bool intersect_aabb(vec3 aabb_min, vec3 aabb_max, vec3 ray_origin, vec3 ray_inverse_direction)
{
	vec3 t_aabb_min = (aabb_min - ray_origin) * ray_inverse_direction;
	vec3 t_aabb_max = (aabb_max - ray_origin) * ray_inverse_direction;

	vec3 t_near_vec = min(max(t_aabb_min, neg_inf), max(t_aabb_max, neg_inf));
	vec3 t_far_vec = max(min(t_aabb_min, inf), min(t_aabb_max, inf));

	float t_near = max3(t_near_vec);
	float t_far = min3(t_far_vec);

	return t_far >= t_near && t_far >= 0; // >= allows for corners to return true (t_near and t_far would be equal in this case)
}


// Simple update
void main() {
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	vec3 pixel_coords_v3 = vec3(pixel_coords.x, pixel_coords.y, 0.0);
	vec3 image_size = vec3(512.0, 512.0, 0.0);
	pixel_coords_v3 = (pixel_coords_v3 - (image_size / 2.0)) / (image_size / 2.0);

	vec3 ray_direction = normalize(vec3(-1.0, -1.0, -1.0));
	vec3 ray_up = normalize(vec3(0.0, 1.0, 0.0));
	vec3 ray_right = cross(ray_direction, ray_up);
	vec3 ray_origin = vec3(1.0, 1.0, 1.0) + ray_right * pixel_coords_v3.x + ray_up * pixel_coords_v3.y;

	vec3 aabb_min = vec3(-0.5);
	vec3 aabb_max = vec3(0.5);

	bool is_intersection = intersect_aabb(
		aabb_min, 
		aabb_max, 
		ray_origin,
		1.0 / ray_direction
	);

	vec4 pixel = vec4(0.0, 0.0, 0.0, 1.0);

	if (is_intersection)
	{
		pixel.r = 1.0;
	}
	else
	{
		pixel.b = 1.0;
	}

	// output to a specific pixel in the image
	imageStore(img_output, pixel_coords, pixel);
}