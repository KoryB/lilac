#version 430

layout(local_size_x = WORKGROUP_SIZE_X, local_size_y = WORKGROUP_SIZE_Y, local_size_z = WORKGROUP_SIZE_Z) in;

struct AABB
{
	vec4 min;
	vec4 max;
};

layout(std430, binding = 0) buffer Pixels
{
	vec4 colors[];
};

layout(std430, binding = 1) buffer InputAABBs
{
	AABB input_aabbs[];
};


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
float intersect_aabb(AABB aabb, vec3 ray_origin, vec3 ray_inverse_direction)
{
	vec3 t_aabb_min = (aabb.min.xyz - ray_origin) * ray_inverse_direction;
	vec3 t_aabb_max = (aabb.max.xyz - ray_origin) * ray_inverse_direction;

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

vec3 pixel_coords_to_camera_coords(vec2 pixel_coords, vec2 image_size, vec3 camera_forward, vec3 camera_up, vec3 camera_origin)
{
	vec2 half_image_size = image_size / 2.0;
	vec2 local_coords_v2 = (pixel_coords - half_image_size) / half_image_size;
	vec3 local_coords = vec3(local_coords_v2, 0.0);

	vec3 camera_right = cross(camera_forward, camera_up);
	vec3 local_up = cross(camera_right, camera_forward);
	vec3 camera_coords = camera_origin + camera_right * local_coords.x + local_up * local_coords.y;

	return camera_coords;
}

vec4 get_aabb_center(AABB aabb)
{
	return (aabb.min + aabb.max) / 2.0;
}

vec4 get_aabb_extents(AABB aabb)
{
	return aabb.max - aabb.min;
}


// Simple update
void main() {
	// get index in global work group i.e x,y position
	uint work_group_size = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;
	vec2 pixel_coords = vec2(gl_WorkGroupID.xy) + vec2(gl_LocalInvocationID.xy) / vec2(gl_WorkGroupSize.xy);

	vec3 camera_forward = normalize(vec3(-1.0, -1.0, -1.0));
	vec3 camera_up = normalize(vec3(0.0, 1.0, 0.0));
	vec3 camera_origin = vec3(2.0, 2.0, 2.0);

	vec3 ray_direction = camera_forward;
	vec3 ray_inv_direction = 1.0 / ray_direction;
	vec3 ray_origin = pixel_coords_to_camera_coords(pixel_coords, gl_NumWorkGroups.xy, camera_forward, camera_up, camera_origin);

	vec3 light_position = camera_origin;
	vec3 light_color = vec3(1.0, 0.0, 0.0);

	float t_hit = 1000000; // TODO: Make max value for raytracer
	int hit_index = -1;

	for (int i = 0; i < INPUT_AABB_SIZE_X * INPUT_AABB_SIZE_Y; i++)
	{
		AABB box = input_aabbs[i];

		float t_hit_box = intersect_aabb(box, ray_origin, ray_inv_direction);
		bool is_closer = t_hit_box > 0 && t_hit_box < t_hit;

		t_hit = float(!is_closer) * t_hit + float(is_closer) * t_hit_box;
		hit_index = int(!is_closer) * hit_index + int(is_closer) * i;
	}

	vec3 color = vec3(0.0);

	if (hit_index > -1)
	{
		AABB hit_aabb = input_aabbs[hit_index];
		vec3 center = get_aabb_center(hit_aabb).xyz;
		vec3 extents = get_aabb_extents(hit_aabb).xyz;

		vec3 hit_absolute = ray_origin + ray_direction * t_hit;
		vec3 hit_relative = hit_absolute - center;
		vec3 hit_normalized = hit_relative / extents;

		int face_index = get_face_index(hit_normalized);
		vec3 face_normal = aabb_normals[face_index];
		vec3 to_light = normalize(light_position - hit_absolute);

		color += clamp(dot(face_normal, to_light) * light_color, 0.0, 1.0);
	}

	vec4 pixel = vec4(color, 1.0);

	uint pixel_index = gl_WorkGroupID.x + gl_WorkGroupID.y * gl_NumWorkGroups.x;
	uint local_index = gl_LocalInvocationID.x + gl_LocalInvocationID.y * gl_WorkGroupSize.x;
	colors[work_group_size*pixel_index + local_index] = pixel;
}