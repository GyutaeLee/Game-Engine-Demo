#version 330 core

in vec3 v_pos;
in vec3 v_normal;
in vec2 v_uv;
in mat3 tbn_mat;

layout (location = 0) out vec4 frag_color;

uniform bool is_use_tangent;

uniform vec3 cam_pos;

uniform vec3 sun_dir;
uniform vec3 sun_ambient;
uniform vec3 sun_diffuse;
uniform vec3 sun_specular;

uniform sampler2D diffuse_texture;
uniform sampler2D normal_texture;

uniform vec3 mat_ambient;
uniform vec3 mat_diffuse;
uniform vec3 mat_specular;
uniform float mat_shininess;

void main()
{
	vec3 light_dir = normalize(sun_dir);
	vec3 view_dir = normalize(cam_pos - v_pos);

	vec4 diffuse_tex_color = texture(diffuse_texture, v_uv);

	vec3 ambient_color = sun_ambient * mat_ambient * diffuse_tex_color.xyz;

	vec3 normal = v_normal;
	if (is_use_tangent)
	{
		// Normal Mapping : get new normal, and then transform it into world space.
		normal = texture(normal_texture, v_uv).xyz;
		normal = normalize(normal * 2.0 - 1.0);
		normal = tbn_mat * normal;
	}
	normal = normalize(normal);

	float diff = max(dot(normal, light_dir), 0.0);
	vec3 diffuse_color = sun_diffuse * mat_diffuse * diff * diffuse_tex_color.xyz;

	vec3 halfway_dir = normalize(light_dir + view_dir);
	float spec = pow(max(dot(normal, halfway_dir), 0.0), mat_shininess);
	vec3 specular_color = sun_specular * mat_specular * spec;

	vec4 lighting_color = vec4(ambient_color + diffuse_color + specular_color, 1.0);
	lighting_color.a *= diffuse_tex_color.a;

	frag_color = lighting_color;
}