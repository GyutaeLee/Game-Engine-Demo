#version 330 core
layout(location = 0) in vec4 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_tangent;
layout(location = 3) in vec2 a_uv;

out vec3 v_pos;
out vec3 v_normal;
out vec2 v_uv;
out mat3 tbn_mat;

uniform mat4 world_mat;
uniform mat4 view_mat;
uniform mat4 projection_mat;

uniform bool is_use_tangent;

void main()
{
	v_pos = vec3((world_mat * a_pos).xyz);
	v_normal = mat3(world_mat) * a_normal;
	v_uv = a_uv;
	gl_Position = projection_mat * view_mat * vec4(v_pos, 1.0);

	if (is_use_tangent)
	{
		// Gram-scmidt Process
		vec3 T = normalize(vec3(world_mat * vec4(a_tangent, 0.0)));
		vec3 N = normalize(v_normal);
		T = normalize(T - dot(T, N) * N);
		vec3 B = cross(N, T);
		tbn_mat = mat3(T, B, N);

		// and then use the normal/depth map in fragment shader with TBNmat
	}
}