#version 430

layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;


const int face_right = 0;
const int face_left = 1;
const int face_up = 2;
const int face_down = 3;
const int face_front = 4;
const int face_back = 5;

const vec3 inf3 = vec3(1.0) / vec3(0.0);
const vec3 neg_inf3 = vec3(-1.0) / vec3(0.0);

const float inf = 1.0 / 0.0;
const float neg_inf = -1.0 / 0.0;

const vec3 aabb_normals[6] = vec3[6](
	vec3(1.0, 0.0, 0.0),
	vec3(-1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, -1.0, 0.0),
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 0.0, -1.0)
);


float min3(vec3 v)
{
	return min(min(v.x, v.y), v.z);
}

float max3(vec3 v)
{
	return max(max(v.x, v.y), v.z);
}

int argmax(vec3 v)
{
	bool xy = v.x >= v.y;
	bool xz = v.x >= v.z;
	bool yz = v.y >= v.z;

	bool ybig = yz && !xy;
	bool zbig = !(xz || yz);

	return 1 * int(ybig) + 2 * int(zbig);
}


// https://tavianator.com/2011/ray_box.html
// The weird seeming min(max(x, neg_inf)) stuff is to prevent the issue
// where 0.0 * inf = NaN, and NaN's are propigated based on the first argument 
// in standard implementations
// see: https://tavianator.com/2022/ray_box_boundary.html
float intersect_aabb(vec3 aabb_min, vec3 aabb_max, vec3 ray_origin, vec3 ray_inverse_direction)
{
	vec3 t_aabb_min = (aabb_min - ray_origin) * ray_inverse_direction;
	vec3 t_aabb_max = (aabb_max - ray_origin) * ray_inverse_direction;

	vec3 t_near_vec = min(max(t_aabb_min, neg_inf), max(t_aabb_max, neg_inf));
	vec3 t_far_vec = max(min(t_aabb_min, inf), min(t_aabb_max, inf));

	float t_near = max3(t_near_vec);
	float t_far = min3(t_far_vec);

	float is_hit = float(t_far >= t_near && t_far >= 0); // >= allows for corners to return true (t_near and t_far would be equal in this case)
	float is_near_behind = float(t_near <= 0);
	float is_near_in_front = float(t_near > 0);

	float t_hit = is_hit * (t_near * is_near_in_front + t_far * is_near_behind); // hit is near if in front, else use t_far

	return t_hit;
}

int get_face_index(vec3 p)
{
	int axis_index = argmax(abs(p));
	int face_index = 2*axis_index + int(p[axis_index] < 0);

	return face_index;
}


// Simple update
void main() {
	// get index in global work group i.e x,y position
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	vec3 pixel_coords_v3 = vec3(pixel_coords.x, pixel_coords.y, 0.0);
	ivec2 image_size = imageSize(img_output);
	pixel_coords_v3 = (pixel_coords_v3 - float(image_size / 2)) / float(image_size / 2);

	vec3 ray_direction = normalize(vec3(-1.0, -1.0, -1.0));
	vec3 ray_up = normalize(vec3(0.0, 1.0, 0.0));
	vec3 ray_right = cross(ray_direction, ray_up);
	vec3 ray_origin = vec3(1.0, 1.0, 1.0) + ray_right * pixel_coords_v3.x + ray_up * pixel_coords_v3.y;

	vec3 light_position = vec3(1.0, 1.0, 1.0);
	vec3 light_color = vec3(1.0, 0.0, 0.0);

	vec3 aabb_min = vec3(-0.5);
	vec3 aabb_max = vec3(0.5);
	vec3 aabb_center = (aabb_min + aabb_max) / 2.0;
	vec3 aabb_extents = aabb_max - aabb_min;

	float t_hit = intersect_aabb(
		aabb_min, 
		aabb_max, 
		ray_origin,
		1.0 / ray_direction
	);

	vec3 hit_absolute = ray_origin + ray_direction * t_hit;
	vec3 hit_relative = hit_absolute - aabb_center;
	vec3 hit_normalized = hit_relative / aabb_extents;

	int face_index = get_face_index(hit_normalized);
	vec3 face_normal = aabb_normals[face_index];
	vec3 to_light = normalize(light_position - hit_absolute);

	vec3 color = vec3(0.0);

	if (t_hit > 0)
	{
		color += clamp(dot(face_normal, to_light) * light_color, 0.0, 1.0);
	}

	vec4 pixel = vec4(color, 1.0);
	// output to a specific pixel in the image
	imageStore(img_output, pixel_coords, pixel);
}