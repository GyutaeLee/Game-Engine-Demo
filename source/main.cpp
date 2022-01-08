#include <stdio.h>
#include <unordered_map>
#include <string>

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "imgui/imgui.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/DefaultLogger.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "utility.h"

#if defined(_WIN32) || defined(_WIN64) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <direct.h>
#endif

GLFWwindow* g_window = NULL;
constexpr int INITIAL_WINDOW_WIDTH = 1250;
constexpr int INITIAL_WINDOW_HEIGHT = 700;
int g_window_width = INITIAL_WINDOW_WIDTH;
int g_window_height = INITIAL_WINDOW_HEIGHT;
double g_time = 0.0;

void do_your_gui_code();

void camera_reset();
void camera_update();

void process_scene_mesh(const aiScene* scene);
void process_scene_material(const aiScene* scene, const char* base_folder);
void model_init();
void model_terminate();
void model_draw();

void imgui_init();
void imgui_terminate();
void imgui_prepare();
void imgui_draw();

void imgui_glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void imgui_glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void imgui_glfw_char_callback(GLFWwindow* window, unsigned int c);
const char* imgui_glfw_clipboard_get(void* user_data);
void imgui_glfw_clipboard_set(void* user_data, const char* text);

void glfw_init();
void glfw_terminate();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main(void)
{
#if defined(_WIN32) || defined(_WIN64) || defined(_DEBUG)
	// https://docs.microsoft.com/en-us/visualstudio/debugger/finding-memory-leaks-using-the-crt-library?view=vs-2019
	// Memory Leak Check on Windows using Crt memomry allocation
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	glfw_init();
	imgui_init();
	model_init();

	while (!glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		// ImGui Data �غ�
		{
			imgui_prepare();		// ImGui�� ���ο� �������� ���� display,time,interaction ���� ����
			ImGui::NewFrame();		// ���ο� �������� ���� ImGui ���� ����
			{
				do_your_gui_code(); // ImGui �ڵ� �ۼ�
			}
			ImGui::Render();		// ImGui ������ ������ ����
		}

		camera_update();

		// ImGui Data ������
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		model_draw();
		imgui_draw();					// ImGUi�� ������ �����͸� GPU�� �÷��� ó���Ѵ�.

		glfwSwapBuffers(g_window);
	}

	model_terminate();
	imgui_terminate();
	glfw_terminate();

	return 0;
}

struct Camera
{
	float move_speed;
	float mouse_sensitivity;

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 forward;

	// view space matrix
	glm::mat4 view;

	// clip space matrix using Perspective Projection
	float fov_degree;
	float near_plane;
	float far_plane;
	glm::mat4 projection;
}g_camera;

void camera_reset()
{
	// ���Ƿ� move speed�� mouse sensitivity�� �����ϰ� ����
	g_camera.move_speed = 50.0f;
	g_camera.mouse_sensitivity = 0.07f;

	// ī�޶� ��ġ�� ȸ������ ��������.
	g_camera.position = glm::vec3(0.0f, 5.0f, 10.0f);
	g_camera.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	// rotation (1, 0, 0, 0)�� �״�� ������ �ٶ󺸰� �����Ƿ�, right/up/forward�� 3���� ���������� ������ ����.
	g_camera.right = glm::vec3(1.0f, 0.0f, 0.0f);
	g_camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_camera.forward = glm::vec3(0.0f, 0.0f, 1.0f);

	// ������ rotation�� position�� ���� view matrix�� ����
	g_camera.view = glm::mat4_cast(glm::conjugate(g_camera.rotation));
	g_camera.view[3][0] = -(g_camera.view[0][0] * g_camera.position.x + g_camera.view[1][0] * g_camera.position.y + g_camera.view[2][0] * g_camera.position.z);
	g_camera.view[3][1] = -(g_camera.view[0][1] * g_camera.position.x + g_camera.view[1][1] * g_camera.position.y + g_camera.view[2][1] * g_camera.position.z);
	g_camera.view[3][2] = -(g_camera.view[0][2] * g_camera.position.x + g_camera.view[1][2] * g_camera.position.y + g_camera.view[2][2] * g_camera.position.z);

	// perspective projection�� ���� fov�� near/far plane ���� �����ϰ�, projection matrix�� ����.
	g_camera.fov_degree = 60.0f;
	g_camera.near_plane = 0.1f;
	g_camera.far_plane = 1000.0f;

	float aspect = (float)g_window_width;
	if (g_window_height != 0)
	{
		// handle with when the window minimized
		aspect /= g_window_height;
	}

	g_camera.projection = glm::perspective(glm::radians(g_camera.fov_degree), aspect, g_camera.near_plane, g_camera.far_plane);
}

void camera_update()
{
	// Window���� Input Data�� glfw�ε� ������ �� ������,
	// �츮�� �� �����͸� ImGui�� �� �־��ְ� �ֱ� ������
	// �׳� ���ϰ� ImGui�� ����ϵ��� �Ѵ�.
	// ���ϸ� glfw�ε� ������ �� �ִ�.
	ImGuiIO& io = ImGui::GetIO();

	bool should_update_view_matrix = false;

	// ���� ���콺 ������ Ŭ���Ǿ��ִ��� && �׸��� ImGui UI�� Ŭ���� �ȵǾ� �ִ���
	// io.WantCaptureMouse�� ���� ������ �ּ� ������ ��.
	if (io.MouseDown[GLFW_MOUSE_BUTTON_LEFT] && !io.WantCaptureMouse)
	{
		should_update_view_matrix = true;

		// mouse�������� delta���� degree��� �����ϰ�, radian������ �ٲٰ� mouse sensitivitiy���� ���Ͽ�
		// ���ϴ� �ΰ����� �����̰� �Ѵ�.
		float x_delta = glm::radians(io.MouseDelta.x) * g_camera.mouse_sensitivity;
		float y_delta = glm::radians(io.MouseDelta.y) * g_camera.mouse_sensitivity;

		// ����ڰ� x������ ���콺�� �������ٸ� y�࿡ ���� ȸ���� ������ �Ѵ�.
		// �� �� ���������� �������ٸ� ���������� ȸ�� ���Ѿ� �ϴµ�, x_delta�� ����ε�, 3���� ȸ������ ����� ������ ȸ����Ű��
		// �������� ȸ���ϹǷ� ���������� �ٲپ��ش�.
		glm::quat y_rot = glm::angleAxis(-x_delta, glm::vec3(0.f, 1.f, 0.f));

		// ����ڰ� y������ ���콺�� �������ٸ� x�࿡ ���� ȸ���� ������ �Ѵ�.
		glm::quat x_rot = glm::angleAxis(-y_delta, glm::vec3(1.f, 0.0, 0.f));

		/*
			Rotation Order : Y -> Previous Rotation -> X
			Previous Rotation is also Y -> X
			��������� ��ü ������ Y -> Y -> X -> X �̹Ƿ�, ���������� Y -> X�̴�.

			�׷��� ���� ȸ���� ���� delta roation�� ���ϴ°� ����������.

			������ ���ĵ��� ���� �����غ����ν� ������ ���ϴ� ����, Ư�� Y -> Y�� �� ȸ���࿡ ���� ȸ���� ������Ű������ �� �� �ִ�
			float ya1 = 31.f;
			float ya2 = 78.f;
			glm::quat y1 = glm::angleAxis(glm::radians(ya1), glm::vec3(0.f, 1.f, 0.f));
			glm::quat y2 = glm::angleAxis(glm::radians(ya2), glm::vec3(0.f, 1.f, 0.f));
			glm::quat y3 = glm::angleAxis(glm::radians(ya1 + ya2), glm::vec3(0.f, 1.f, 0.f));
			glm::quat ycomb = y1 * y2;
			glm::quat ycomb_reverse = y2 * y1;

			float xa1 = 3.f;
			float xa2 = 142.f;
			glm::quat x1 = glm::angleAxis(glm::radians(xa1), glm::vec3(1.f, 0.f, 0.f));
			glm::quat x2 = glm::angleAxis(glm::radians(xa2), glm::vec3(1.f, 0.f, 0.f));
			glm::quat x3 = glm::angleAxis(glm::radians(xa1 + xa2), glm::vec3(1.f, 0.f, 0.f));
			glm::quat xcomb = x1 * x2;
			glm::quat xcomb_reverse = x2 * x1;

			glm::quat new_y_rot = glm::angleAxis(glm::raidnas(30.f), glm::vec3(0.f, 1.f, 0.f));
			glm::quat new_x_rot = glm::angleAxis(glm::raidnas(16.f), glm::vec3(1.f, 0.f, 0.f));
			glm::quat r1 = ((new_y_rot * (y1 * x1)) * new_x_rot);
			glm::quat r2 = ((y1 * (new_y_rot * x1)) * new_x_rot);
		*/

		// mouse ������ delta ���� ���� ī�޶��� ���ο� ȸ�� ����
		g_camera.rotation = (y_rot * g_camera.rotation) * x_rot;

		// ȸ���� ���ŵǾ����Ƿ�, quat�� matrix���·� �ٲپ� �ش� ��ǥ���� �������� right/up/forward�� �� �־��ش�.
		glm::mat3 rot_mat = glm::mat3_cast(g_camera.rotation);
		g_camera.right = rot_mat[0];
		g_camera.up = rot_mat[1];
		g_camera.forward = rot_mat[2];
	}

	// ����ڰ� ImGUi�� Ű���� ��ȣ�ۿ��� �ϰ� ���� ���� ��, �ڼ��� ���� �ּ� ����.
	if (!io.WantCaptureKeyboard)
	{
		// �̹� �������� delta time�� ���� move speed�� ���� ���ϴ� �ӵ��� �������ְ� ���ش�.
		// delta time�� � �����ӿ������� ������ �ӵ��� ���� ���� ���̴�.
		float delta_move = io.DeltaTime * g_camera.move_speed;

		// ����ڰ� shiftŰ�� �����ٸ� 10�� ���� ������ �ӵ��� ���ӽ����ش�.
		if (io.KeyShift)
		{
			delta_move *= 10.f;
		}

		// ����ڰ� ���� Ű���忡 ���� �Ϲ����� FPS ī�޶��� �������� ������ش�.
		if (io.KeysDown[GLFW_KEY_W])
		{
			should_update_view_matrix = true;
			g_camera.position -= g_camera.forward * delta_move;
		}

		if (io.KeysDown[GLFW_KEY_A])
		{
			should_update_view_matrix = true;
			g_camera.position -= g_camera.right * delta_move;
		}

		if (io.KeysDown[GLFW_KEY_S])
		{
			should_update_view_matrix = true;
			g_camera.position += g_camera.forward * delta_move;
		}

		if (io.KeysDown[GLFW_KEY_D])
		{
			should_update_view_matrix = true;
			g_camera.position += g_camera.right * delta_move;
		}
	}

	// ���� camera�� position/rotation �� �� �ϳ��� ������Ʈ �Ǿ��ٸ�
	// view matrix�� ������Ʈ ���ش�.
	if (should_update_view_matrix)
	{
		// camera�� ȸ���� �Ͱ� ���������� ��� �ٸ� �繰���� �������� �ϱ� ������
		// quaternion conjugate�� �ݴ�������� ȸ�������ش�.
		// �׸��� camera position�� �� rotation ��ǥ�踦 ���� view[3] position�ڸ��� �־��ش�.
		g_camera.view = glm::mat4_cast(glm::conjugate(g_camera.rotation));
		g_camera.view[3][0] = -(g_camera.view[0][0] * g_camera.position.x + g_camera.view[1][0] * g_camera.position.y + g_camera.view[2][0] * g_camera.position.z);
		g_camera.view[3][1] = -(g_camera.view[0][1] * g_camera.position.x + g_camera.view[1][1] * g_camera.position.y + g_camera.view[2][1] * g_camera.position.z);
		g_camera.view[3][2] = -(g_camera.view[0][2] * g_camera.position.x + g_camera.view[1][2] * g_camera.position.y + g_camera.view[2][2] * g_camera.position.z);
	}

	float aspect = (float)g_window_width;
	if (g_window_height != 0)
	{
		// handle with when the window minimized
		aspect /= g_window_height;
	}

	// perspective�� �������ش�. �̰��� ���� ���� �ٲ� ���� ������Ʈ ���൵ �Ǵµ�
	// window dimension ���ŵǴ� ���� ������ �ΰ� Ȯ���ϱⰡ �����Ƽ� �׳� �̷��� �� �����Ӹ��� �����ϰ� �д�.
	g_camera.projection = glm::perspective(glm::radians(g_camera.fov_degree), aspect, g_camera.near_plane, g_camera.far_plane);
}

struct DirectionalLight
{
	glm::vec3 rot_euler;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct Mesh
{
	std::vector<float> position;
	std::vector<float> normal;
	std::vector<float> tangent;
	std::vector<float> uv;
	std::vector<uint32_t> indices;

	// Model struct���� std::vector<Material> material�� element index�� ����Ų��.
	// ���� 0 �̻��̾�� ��ȿ�ϰ�, �ƴ϶�� g_default_material�� �Ἥ ������ �ؾ� �Ѵ�.
	int material_index;
};

// �Ϲ����� Phong Lighting Model�� ���� ������ �� �� �ִ� Material����.
struct Material
{
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float shininess;

	bool is_transparent;
	bool two_sided; //face culling�� ���� �� ���� �ʴ���, Ȥ�� �ϴ���

	// diffuse texture�� ���� Material�� ��� g_default_white_texture ���� �־�����.
	GLuint gl_diffuse;

	// normal mapping�� ���� ���ȴ�.
	bool has_normal_texture;
	GLuint gl_normal;

	char debug_mat_name[64];
};

struct Model
{
	// Model data ����
	std::vector<Mesh> mesh;
	std::vector<Material> material;

	// �� Mesh�� � ������ �������ؾ� �� ��,
	// �� vector�� �ִ� ���� std::vector<Mesh> mesh�� �����ϴµ� ���ȴ�.
	std::vector<unsigned> draw_order;

	// Model Rendering�� �̿�Ǵ� PSO(Pipeline State Object) + Buffers
	GLuint shader_vertex;
	GLuint shader_frag;
	GLuint pso;
	std::vector<GLuint> vaos;	// vertex array object
	std::vector<GLuint> vbos;	// vertex buffer object
	std::vector<GLuint> ibos;	// index buffer object

	// uniform locations
	GLint loc_world_mat;
	GLint loc_view_mat;
	GLint loc_projection_mat;
	GLint loc_is_use_tangent;
	GLint loc_cam_pos;
	GLint loc_sun_dir;
	GLint loc_sun_ambient;
	GLint loc_sun_diffuse;
	GLint loc_sun_specular;
	GLint loc_diffuse_texture;
	GLint loc_normal_texture;
	GLint loc_mat_ambient;
	GLint loc_mat_diffuse;
	GLint loc_mat_specular;
	GLint loc_mat_shininess;

	// Model�� transform ����.
	// rotation�� ��� Unityó�� �� xyz�� Euler Angle�� ��Ÿ����.
	glm::vec3 scale;
	glm::vec3 rot_euler;
	glm::vec3 position;
}g_model;

DirectionalLight g_light;
Material g_default_material;
GLuint g_default_texture_white;

// Model Transform�� Material�� �ʱ� ���� (Material�� ��� �� �����Ϳ� ������ �����Ͱ� ���� ��츦 ����)
constexpr glm::vec3 INITIAL_MODEL_SCALE(1.0f);
constexpr glm::vec3 INITIAL_MODEL_ROTATION(0.0f);	// xyz�� Euler Angle�� ��Ÿ����.
constexpr glm::vec3 INITIAL_MODEL_POSITION(0.0f);
constexpr glm::vec3 INITIAL_MATERIAL_AMBIENT(0.0f);
constexpr glm::vec3 INITIAL_MATERIAL_DIFFUSE(1.f);
constexpr glm::vec3 INITIAL_MATERIAL_SPECULAR(1.f);
constexpr float INITIAL_MATERIAL_SHININESS = 0.f;
constexpr glm::vec3 INITIAL_LIGHT_ROT_EULER = glm::vec3(65.f, 45.f, 32.f);
constexpr glm::vec3 INITIAL_LIGHT_AMBIENT = glm::vec3(0.1f);
constexpr glm::vec3 INITIAL_LIGHT_DIFFUSE = glm::vec3(0.5f);
constexpr glm::vec3 INITIAL_LIGHT_SPECULAR = glm::vec3(0.2f);

void process_scene_mesh(const aiScene* scene)
{
	assert(scene != nullptr &&
		scene->mMeshes != nullptr &&
		scene->mNumMeshes != 0);

	g_model.mesh.resize(scene->mNumMeshes);

	for (int mesh_index = 0; mesh_index < scene->mNumMeshes; ++mesh_index)
	{
		const aiMesh* ai_mesh = scene->mMeshes[mesh_index];

		// assimp�� �� ������ �о� ���� ��, normal/tangent�� �����϶�� �����Ƿ�
		// �ʿ��� �͵��� ��� �����ϴ��� Ȯ���Ѵ�.
		assert(ai_mesh->HasPositions() == true &&
			ai_mesh->HasNormals() == true &&
			ai_mesh->HasTangentsAndBitangents() == true &&
			ai_mesh->HasFaces() == true);

		/*
		   assimp�κ��� mesh data�� �����Ƿ�, �װ��� �츮�� ���ϴ� ���¿� ���缭 �����Ѵ�.
		   ���� shader���� ������ ���� ��µ�,
		   layout(location = 0) in vec4 a_pos;
		   layout(location = 1) in vec3 a_normal;
		   layout(location = 2) in vec3 a_tangent;
		   layout(location = 3) in vec2 a_uv;

		   Buffer�� ���������� �ξ
		   pos x y z w / x y z w / ...
		   norrmal x y z / x y z / ...
		   tangent x y z / x y z / ...
		   uv x y / x y / ...
		   �� �ξ �������� �����̴�.

		   ��� ���⿡�� �ٷ� GL Buffer���� ���� ������, ������ ��Ȯ�� �ϱ� ���� assimpó�� �ܰ�
		   GL ó������ ��������.
	   */
		
		Mesh* my_mesh = &(g_model.mesh[mesh_index]);

		// mesh�� ������ �ִ� ���� ������ ���� �� CPU Buffer���� �̸� �Ҵ�.
		my_mesh->position.resize(ai_mesh->mNumVertices * 4);
		my_mesh->normal.resize(ai_mesh->mNumVertices * 3);
		my_mesh->tangent.resize(ai_mesh->mNumVertices * 3);
		my_mesh->uv.resize(ai_mesh->mNumVertices * 2);
		for (unsigned ai_vertex_index = 0; ai_vertex_index < ai_mesh->mNumVertices; ++ai_vertex_index)
		{
			unsigned access_index = ai_vertex_index * 4;
			my_mesh->position[access_index++] = ai_mesh->mVertices[ai_vertex_index].x;
			my_mesh->position[access_index++] = ai_mesh->mVertices[ai_vertex_index].y;
			my_mesh->position[access_index++] = ai_mesh->mVertices[ai_vertex_index].z;
			my_mesh->position[access_index] = 1.f;

			access_index = ai_vertex_index * 3;
			my_mesh->normal[access_index++] = ai_mesh->mNormals[ai_vertex_index].x;
			my_mesh->normal[access_index++] = ai_mesh->mNormals[ai_vertex_index].y;
			my_mesh->normal[access_index] = ai_mesh->mNormals[ai_vertex_index].z;

			access_index = ai_vertex_index * 3;
			my_mesh->tangent[access_index++] = ai_mesh->mTangents[ai_vertex_index].x;
			my_mesh->tangent[access_index++] = ai_mesh->mTangents[ai_vertex_index].y;
			my_mesh->tangent[access_index] = ai_mesh->mTangents[ai_vertex_index].z;

			// UV�� �ش� �����Ͱ� �����ϴ����� Ȯ���ϰ�, 0��° ��ǥ���� ����ϵ��� �Ѵ�.
			// �ؽ��� ��ǥ�� �������� ��� ���̴��� �� ���������Ƿ� �����Ͱ� ������ 0���� �־��ش�.
			access_index = ai_vertex_index * 2;

			if (ai_mesh->mTextureCoords[0])
			{
				my_mesh->uv[access_index++] = ai_mesh->mTextureCoords[0][ai_vertex_index].x;
				my_mesh->uv[access_index] = ai_mesh->mTextureCoords[0][ai_vertex_index].y;
			}
			else
			{
				my_mesh->uv[access_index++] = 0.f;
				my_mesh->uv[access_index] = 0.f;
			}
		}

		// face�� �� ���� �̷�� �ﰢ���� �ε����� ���Ѵ�.
		for (unsigned ai_face_index = 0; ai_face_index < ai_mesh->mNumFaces; ++ai_face_index)
		{
			aiFace face = ai_mesh->mFaces[ai_face_index];
			size_t indices_size = my_mesh->indices.size();
			my_mesh->indices.resize(indices_size + face.mNumIndices);
			memcpy(my_mesh->indices.data() + indices_size, face.mIndices, sizeof(unsigned) * face.mNumIndices);
		}

		// assimp�� mesh�� ����Ű�� material index�� �ִٸ�
		// �װ��� mesh struct�� material_index�� �־��ش�.
		// ���ٸ� ���� ���� �־��ش�.
		if (ai_mesh->mMaterialIndex >= 0)
		{
			my_mesh->material_index = ai_mesh->mMaterialIndex;
		}
		else
		{
			my_mesh->material_index = -1;
		}
	}
}

void process_scene_material(const aiScene* scene, const char* base_folder)
{
	// �̹����� �ҷ��� �� ��ο� ���� string ���� ó���� �ϱ� ���� base_folder�� ���̸� ����Ѵ�.
	size_t base_folder_len = strlen(base_folder);

	// ���� Material�� ������ �̹����� �̿��� �� �ֱ� ������
	// ���� �̹����� ���� �� �θ��� ����, �� �� ���ε� �ϸ� �� �̹����� �ٷ� ����� �� �ֵ���
	// Key�� string�̰�, Value�� ImageInfo�� map�� �̿��Ѵ�.
	struct ImageInfo
	{
		int width, height, comp;
		GLuint gl_id;
	};
	std::unordered_map<std::string, ImageInfo> path_texture_map;

	// assimp�κ��� texture path�� ������ �� �̿��ϴ� string
	aiString assimp_str;

	// stbi�� �̿��� path�� ���� �� �̿��ϴ� string
	std::string std_str;

	// Material ������ŭ �̸� �޸� �Ҵ�
	g_model.material.resize(scene->mNumMaterials);
	for (unsigned i = 0; i < scene->mNumMaterials; ++i)
	{
		Material* model_mat = &(g_model.material[i]);

		// Material �ʱ�ȭ
		memset(model_mat, 0, sizeof(Material));

		model_mat->ambient = INITIAL_MATERIAL_AMBIENT;
		model_mat->diffuse = INITIAL_MATERIAL_DIFFUSE;
		model_mat->specular = INITIAL_MATERIAL_SPECULAR;
		model_mat->shininess = INITIAL_MATERIAL_SHININESS;

		model_mat->gl_diffuse = g_default_texture_white;

		aiMaterial* assimp_mat = scene->mMaterials[i];

		aiString mat_name = assimp_mat->GetName();
		size_t name_buffer_size = sizeof(model_mat->debug_mat_name) - 1;
		size_t copy_size = name_buffer_size < mat_name.length ? name_buffer_size : mat_name.length;
		memcpy(model_mat->debug_mat_name, mat_name.C_Str(), copy_size);
		model_mat->debug_mat_name[copy_size] = '\0';

		clock_t start, end;
		start = clock();
		// Diffuse Texture�� �����ϴ���?
		if (unsigned texture_count = assimp_mat->GetTextureCount(aiTextureType_DIFFUSE))
		{
			// �����Ѵٸ� assimp_str�� �ش� texture�� ��θ� �����´�.
			assimp_mat->GetTexture(aiTextureType_DIFFUSE, 0, &assimp_str);

			// assimp_str���� base_folder�� ���ܵ� ä�� ������ ������,
			// ���� ���� ��θ� �߰��� ���� ��θ� �ϼ����ش�.
			std_str.reserve(base_folder_len + 1 + assimp_str.length);
			std_str.assign(base_folder);
			std_str.append("/");
			std_str.append(assimp_str.C_Str());

			// �ش� ����� �̹����� �̹� �ҷ��Դ��� Ȯ���Ѵ�.
			auto ret = path_texture_map.find(std_str);

			ImageInfo info;
			if (ret == path_texture_map.end())
			{
				// �� �̹����� ó�� �ҷ����� ���̹Ƿ� stbi�� ���� file���� memory�� �̹����� �ø���, GPU�� �ø���.
				clock_t intense_start, intense_end;
				intense_start = clock();

				// image�� memory�� �ø���
				unsigned char* data = stbi_load(std_str.c_str(), &info.width, &info.height, &info.comp, 0);
				intense_end = clock();
				printf("stbi load %f\n", (float)(intense_end - intense_start) / CLOCKS_PER_SEC);

				intense_start = clock();

				// �ش� cpu memory�� gpu memory�� �ø���
				info.gl_id = gl_load_model_texture(data, info.width, info.height, info.comp);
				intense_end = clock();
				printf("GPU upload %f\n", (float)(intense_end - intense_start) / CLOCKS_PER_SEC);

				// map�� key/value �߰�
				path_texture_map[std_str] = info;
				intense_start = clock();
				
				// gpu�� �÷����Ƿ� cpu���� �޸� ����
				stbi_image_free(data);
				intense_end = clock();
				printf("stbi image free %f\n", (float)(intense_end - intense_start) / CLOCKS_PER_SEC);
			}
			else
			{
				// �ش� ����� �̹����� ������ �ҷ����Ƿ� �� ����� �����´�
				info = ret->second;
			}

			// gpu�� �ö� diffuse�� texture id�� �־��ش�.
			model_mat->gl_diffuse = info.gl_id;

			// image�� component�� alpha channel�� �����ϸ�
			// transparency�� ���� blending�� ����Ϸ� �� �� �ֱ� ������ �ش� material�� flag�� set�Ѵ�.
			if (info.comp >= 4)
			{
				model_mat->is_transparent = true;
			}
		}

		if (unsigned texture_count = assimp_mat->GetTextureCount(aiTextureType_NORMALS))
		{
			assimp_mat->GetTexture(aiTextureType_NORMALS, 0, &assimp_str);

			std_str.reserve(base_folder_len + 1 + assimp_str.length);
			std_str.assign(base_folder);
			std_str.append("/");
			std_str.append(assimp_str.C_Str());

			auto ret = path_texture_map.find(std_str);

			ImageInfo info;
			if (ret == path_texture_map.end())
			{
				clock_t intense_start, intense_end;

				intense_start = clock();
				unsigned char* data = stbi_load(std_str.c_str(), &info.width, &info.height, &info.comp, 0);
				intense_end = clock();

				printf("stbi load %f\n", (float)(intense_end - intense_start) / CLOCKS_PER_SEC);

				intense_start = clock();
				info.gl_id = gl_load_model_texture(data, info.width, info.height, info.comp);
				intense_end = clock();
				printf("GPU upload %f\n", (float)(intense_end - intense_start) / CLOCKS_PER_SEC);

				path_texture_map[std_str] = info;

				intense_start = clock();
				stbi_image_free(data);
				intense_end = clock();
				printf("stbi image free %f\n", (float)(intense_end - intense_start) / CLOCKS_PER_SEC);
			}
			else
			{
				info = ret->second;
			}

			model_mat->has_normal_texture = true;
			model_mat->gl_normal = info.gl_id;

			if (info.comp >= 4)
			{
				model_mat->is_transparent = true;
			}
		}
		end = clock();
		printf("Mat %u : Texture Processing Time %f\n", i, (float)(end - start) / CLOCKS_PER_SEC);

		// lighting map ���� material color value�� lighting parameter���� ��ȸ�Ѵ�.
		constexpr float alpha_threshold = 0.0001f;
		aiColor4D diffuse;
		if (AI_SUCCESS == aiGetMaterialColor(assimp_mat, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
		{
			model_mat->diffuse.x = diffuse.r;
			model_mat->diffuse.y = diffuse.g;
			model_mat->diffuse.z = diffuse.b;

			if (diffuse.a > alpha_threshold && diffuse.a < 1.0f)
			{
				model_mat->is_transparent = true;
			}
		}

		aiColor4D specular;
		if (AI_SUCCESS == aiGetMaterialColor(assimp_mat, AI_MATKEY_COLOR_SPECULAR, &specular))
		{
			model_mat->specular.x = specular.r;
			model_mat->specular.y = specular.g;
			model_mat->specular.z = specular.b;

			if (specular.a > alpha_threshold && specular.a < 1.f)
			{
				model_mat->is_transparent = true;
			}
		}

		aiColor4D ambient;
		if (AI_SUCCESS == aiGetMaterialColor(assimp_mat, AI_MATKEY_COLOR_AMBIENT, &ambient))
		{
			model_mat->ambient.x = ambient.r;
			model_mat->ambient.y = ambient.g;
			model_mat->ambient.z = ambient.b;

			if (ambient.a > alpha_threshold && ambient.a < 1.f)
			{
				model_mat->is_transparent = true;
			}
		}

		// phong model lighting�� specular ���꿡 �̿�Ǵ� shininess ��
		ai_real shininess, strength;
		unsigned int max;
		if (AI_SUCCESS == aiGetMaterialFloatArray(assimp_mat, AI_MATKEY_SHININESS, &shininess, &max))
		{
			model_mat->shininess = shininess;

			if (AI_SUCCESS == aiGetMaterialFloatArray(assimp_mat, AI_MATKEY_SHININESS_STRENGTH, &strength, &max)) {
				model_mat->shininess *= strength;
			}
		}

		// � material�� ��� ����� �� �� ������ �Ǿ�� �ϴ� ��쵵 �����Ƿ� �̰͵� ��ȸ�ؼ� material�� �־��ش�.
		int is_two_sided = 0;
		if (AI_SUCCESS == aiGetMaterialIntegerArray(assimp_mat, AI_MATKEY_TWOSIDED, &is_two_sided, &max))
		{
			model_mat->two_sided = is_two_sided;
		}
	}
}

void model_init()
{
#if defined(_WIN32) || defined(_WIN64)
	// model�ҷ��������� ���� �� exe�� ��� ��θ� ������� ����θ� �ҷ��� �� �ִ���
	// Ȯ���ϱ� ���� üũ
	char* cwd = _getcwd(NULL, 0);
	printf("Current Dir : %s\n", cwd);
	free(cwd);
#endif
	{
		// default white texture ����
		unsigned char white[4] = { 255,255,255,255 };
		glGenTextures(1, &g_default_texture_white);
		glBindTexture(GL_TEXTURE_2D, g_default_texture_white);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		g_light.rot_euler = INITIAL_LIGHT_ROT_EULER;
		g_light.ambient = INITIAL_LIGHT_AMBIENT;
		g_light.diffuse = INITIAL_LIGHT_DIFFUSE;
		g_light.specular = INITIAL_LIGHT_SPECULAR;

		// default material in case of mesh without material
		g_default_material.ambient = INITIAL_MATERIAL_AMBIENT;
		g_default_material.diffuse = INITIAL_MATERIAL_DIFFUSE;
		g_default_material.specular = INITIAL_MATERIAL_SPECULAR;
		g_default_material.shininess = INITIAL_MATERIAL_SHININESS;
		g_default_material.is_transparent = false;
		g_default_material.two_sided = false;
		g_default_material.gl_diffuse = g_default_texture_white;
		g_default_material.has_normal_texture = false;

		g_model.scale = INITIAL_MODEL_SCALE;
		g_model.position = INITIAL_MODEL_POSITION;
		g_model.rot_euler = INITIAL_MODEL_ROTATION;

		// Model Data Handling with Assimp
		{
			// Assimp::DefaultLogger�� assimp library�� ���� �ҷ��� ��
			// ���� ���� �Ͼ�� �ִ����� �� �� �ְ� ���ִ� logger system�̴�.
			// severity�� verbose / debuggin / normal�� ���� �츮�� �α� ����� ������ ������ �� �ִ�.
			Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;

			// stdout�� logger�� �����ϰ� �ؼ� �츮�� �ܼ�â�� �߰� ���ش�.
			Assimp::DefaultLogger::create("", severity, aiDefaultLogStream_STDOUT);

			// �׽�Ʈ ��� singleton�� logger�� �����ͼ� �츮�� �α׸� ��½�Ű�� �Ѵ�.
			Assimp::DefaultLogger::get()->info("this is my info-call");

			clock_t start, end;
			start = clock();

			// exe ���� ��ġ�� ������� resource ������ �� �����͸� �ҷ��´�.
			// �� �� lighting�� normal mapping�� ���� CalcTangentSpace/GenSmoothNormals/GenUVCoords�� ������ش�.
			// FlipUVs�� �Ϲ������� texture uv�� gl�� �� ���� ���� �� �ִµ� �װ��� Flip�Ͽ� �ذ��� �����ϴ�.
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile("resource/rhino/scene.gltf",
				aiProcess_Triangulate |
				aiProcess_FlipUVs |
				aiProcess_CalcTangentSpace |
				aiProcess_GenSmoothNormals |
				aiProcess_GenUVCoords);
			if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			{
				Assimp::DefaultLogger::get()->info(importer.GetErrorString());
				printf("Fail to Read Model File\n");
				assert(false);
			}
			end = clock();

			printf("Assimp Read Time %f seconds\n", (float)(end - start) / CLOCKS_PER_SEC);

			if (scene->HasMaterials())
			{
				start = clock();

				// model file�� material ������ ��ȸ�Ͽ� model data�� ó���Ѵ�.
				process_scene_material(scene, "resource/rhino");

				end = clock();
				printf("Assimp Process Scene Material Time %f seconds\n", (float)(end - start) / CLOCKS_PER_SEC);
			}

			if (scene->HasMeshes())
			{
				start = clock();

				// model file�� mesh�� ��ȸ�Ͽ� model data�� ó���Ѵ�.
				process_scene_mesh(scene);

				end = clock();

				printf("Assimp Process Scene Mesh Time %f seconds\n", (float)(end - start) / CLOCKS_PER_SEC);
			}

			// �� �̻� �θ��� �����Ƿ� logger�� �Ⱦ��ϱ� ����
			Assimp::DefaultLogger::kill();
		}
	}

	{
		// �� �������۴µ� ����� ���̴��� �ҷ��ͼ� pso���� �����Ѵ�.
		std::vector<char> shader_source;

		file_open_fill_buffer("model_shader.vert", shader_source);
		GLuint vso = glCreateShader(GL_VERTEX_SHADER);
		gl_validate_shader(vso, (const char*)shader_source.data());
		g_model.shader_vertex = vso;

		file_open_fill_buffer("model_shader.frag", shader_source);
		GLuint fso = glCreateShader(GL_FRAGMENT_SHADER);
		gl_validate_shader(fso, (const char*)shader_source.data());
		g_model.shader_frag = fso;

		GLuint pso = glCreateProgram();
		gl_validate_program(pso, vso, fso);
		g_model.pso = pso;

		// �ش� ���̴����� uniform���� ��ġ�� �̸� ã�� �����Ѵ�.
		g_model.loc_world_mat = glGetUniformLocation(pso, "world_mat");
		g_model.loc_view_mat = glGetUniformLocation(pso, "view_mat");
		g_model.loc_projection_mat = glGetUniformLocation(pso, "projection_mat");
		g_model.loc_is_use_tangent = glGetUniformLocation(pso, "is_use_tangent");
		g_model.loc_cam_pos = glGetUniformLocation(pso, "cam_pos");
		g_model.loc_sun_dir = glGetUniformLocation(pso, "sun_dir");
		g_model.loc_sun_ambient = glGetUniformLocation(pso, "sun_ambient");
		g_model.loc_sun_diffuse = glGetUniformLocation(pso, "sun_diffuse");
		g_model.loc_sun_specular = glGetUniformLocation(pso, "sun_specular");
		g_model.loc_diffuse_texture = glGetUniformLocation(pso, "diffuse_texture");
		g_model.loc_normal_texture = glGetUniformLocation(pso, "normal_texture");
		g_model.loc_mat_ambient = glGetUniformLocation(pso, "mat_ambient");
		g_model.loc_mat_diffuse = glGetUniformLocation(pso, "mat_diffuse");
		g_model.loc_mat_specular = glGetUniformLocation(pso, "mat_specular");
		g_model.loc_mat_shininess = glGetUniformLocation(pso, "mat_shininess");

		// ������ Mesh���� ���� GL Buffers���� �����Ѵ�.
		constexpr size_t BUFFER_COUNT = 5; // VBO + IBO
		constexpr size_t VBO_COUNT = 4;
		const size_t mesh_count = g_model.mesh.size();
		g_model.vbos.reserve(VBO_COUNT * mesh_count);
		g_model.ibos.reserve(mesh_count);
		g_model.vaos.reserve(mesh_count);

		for (const Mesh& mesh : g_model.mesh)
		{
			// pos / normal / tangent / uv / indicies
			GLuint buffers[BUFFER_COUNT] = { 0,0,0,0,0 };
			glGenBuffers(BUFFER_COUNT, buffers);

			g_model.vbos.push_back(buffers[0]);
			g_model.vbos.push_back(buffers[1]);
			g_model.vbos.push_back(buffers[2]);
			g_model.vbos.push_back(buffers[3]);

			g_model.ibos.push_back(buffers[4]);

			GLuint vao;
			glGenVertexArrays(1, &vao);
			g_model.vaos.push_back(vao);

			glBindVertexArray(vao);

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glEnableVertexAttribArray(3);

			glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)* mesh.position.size(), mesh.position.data(), GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)* mesh.normal.size(), mesh.normal.data(), GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, buffers[2]);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)* mesh.tangent.size(), mesh.tangent.data(), GL_STATIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, buffers[3]);
			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)* mesh.uv.size(), mesh.uv.data(), GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[4]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)* mesh.indices.size(), mesh.indices.data(), GL_STATIC_DRAW);
		}
	}
}

void model_terminate()
{
	// ��� �������� ����.
	glDeleteVertexArrays((GLsizei)g_model.vaos.size(), g_model.vaos.data());
	glDeleteBuffers((GLsizei)g_model.vbos.size(), g_model.vbos.data());
	glDeleteBuffers((GLsizei)g_model.ibos.size(), g_model.ibos.data());
	glDeleteProgram(g_model.pso);
	glDeleteShader(g_model.shader_frag);
	glDeleteShader(g_model.shader_vertex);
}

static bool is_sort_draw_order = true;
void model_draw()
{
	assert(g_model.mesh.size() == g_model.vaos.size());

	// viewport ����
	glViewport(0, 0, g_window_width, g_window_height);

	// 3d rendering�̹Ƿ� depth test�� Ȱ��ȭ���ش�.
	glEnable(GL_DEPTH_TEST);

	// model rendering�� ���� pso ���
	glUseProgram(g_model.pso);

	constexpr glm::mat4 identity(1.0f);

	/* 
	* model�� local to world ��ǥ�踦 �������ش�.
	*/ 
	
	// Rotation Order : Y -> X -> Z
	glm::quat rot = glm::angleAxis(glm::radians(g_model.rot_euler.y), glm::vec3(0.0f, 1.0f, 0.0f)) *
		glm::angleAxis(glm::radians(g_model.rot_euler.x), glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::angleAxis(glm::radians(g_model.rot_euler.z), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 model_transform = glm::translate(identity, g_model.position) *
								glm::mat4_cast(rot) *
								glm::scale(identity, g_model.scale);

	// local to world matrix / world to view matrix / view to clip matrix ������Ʈ ���ְ�,
	// lighting�� ���� position�� ������Ʈ ���ش�.
	glUniformMatrix4fv(g_model.loc_world_mat, 1, GL_FALSE, &(model_transform[0][0]));
	glUniformMatrix4fv(g_model.loc_view_mat, 1, GL_FALSE, &(g_camera.view[0][0]));
	glUniformMatrix4fv(g_model.loc_projection_mat, 1, GL_FALSE, &(g_camera.projection[0][0]));
	glUniform3fv(g_model.loc_cam_pos, 1, &(g_camera.position[0]));

	glm::quat light_rot = glm::angleAxis(glm::radians(g_light.rot_euler.y), glm::vec3(0.0f, 1.f, 0.f)) *
		glm::angleAxis(glm::radians(g_light.rot_euler.x), glm::vec3(1.0f, 0.f, 0.f)) *
		glm::angleAxis(glm::radians(g_light.rot_euler.z), glm::vec3(0.f, 0.f, 1.f));
	glm::vec3 light_dir = -(glm::mat3_cast(light_rot)[2]);
	glUniform3fv(g_model.loc_sun_dir, 1, &(light_dir[0]));
	glUniform3fv(g_model.loc_sun_ambient, 1, &(g_light.ambient[0]));
	glUniform3fv(g_model.loc_sun_diffuse, 1, &(g_light.diffuse[0]));
	glUniform3fv(g_model.loc_sun_specular, 1, &(g_light.specular[0]));

	// diffuse/normal texture�� ���� Texture Image Unit�� �̸� �����صд�.
	glUniform1i(g_model.loc_diffuse_texture, 0);
	glUniform1i(g_model.loc_normal_texture, 1);

	// �������� �޽��� draw_order�� ���� �������Ѵ�.

	// transparent�� mesh rendering�� ��� ���� opaque�� object�� ������ �� �Ŀ� �ؾ��Ѵ�.
	// ���� draw_order�� ���� material�� transparent ���η� opaque�� ���� ���� �������ǰ�,
	// �� ���Ŀ� transparent�� ������ �ǰ� �Ѵ�.
	g_model.draw_order.resize(g_model.mesh.size());
	for (unsigned i = 0; i < g_model.mesh.size(); ++i)
	{
		g_model.draw_order[i] = i;
	}

	if (is_sort_draw_order)
	{
		std::sort(g_model.draw_order.begin(), g_model.draw_order.end(), [](unsigned a, unsigned b) -> bool
			{
				int am_index = g_model.mesh[a].material_index;
				int bm_index = g_model.mesh[b].material_index;
				Material* am = am_index >= 0 ? &(g_model.material[am_index]) : &(g_default_material);
				Material* bm = bm_index >= 0 ? &(g_model.material[bm_index]) : &(g_default_material);
				return am->is_transparent < bm->is_transparent;
			});
	}

	const int mesh_count = (int)g_model.mesh.size();
	for (int i = 0; i < mesh_count; ++i)
	{
		unsigned draw_order = g_model.draw_order[i];
		const Mesh& mesh = g_model.mesh[draw_order];
		GLuint vao = g_model.vaos[draw_order];

		// �������� mehs�� material�� �����´�. ������ default material.
		const Material* mat = &(g_model.material[mesh.material_index]);
		if (mesh.material_index >= 0)
		{
			mat = &(g_model.material[mesh.material_index]);
		}
		else
		{
			mat = &(g_default_material);
		}

		// �̿� ���� ���� texture, uniform data �׸��� rasterization state�� �������ش�.
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mat->gl_diffuse);
		glUniform3fv(g_model.loc_mat_ambient, 1, &(mat->ambient[0]));
		glUniform3fv(g_model.loc_mat_diffuse, 1, &(mat->diffuse[0]));
		glUniform3fv(g_model.loc_mat_specular, 1, &(mat->specular[0]));
		glUniform1f(g_model.loc_mat_shininess, mat->shininess);
		
		if (mat->two_sided)
		{
			glDisable(GL_CULL_FACE);
		}
		else
		{
			glEnable(GL_CULL_FACE);
		}

		// transparent material�̶�� �Ϲ����� blending equation�� ���ش�.
		if (mat->is_transparent)
		{
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			glDisable(GL_BLEND);
		}

		// normal texture�� ������ �ִٸ� normal mapping�� ����
		// ���� unifrom data�� ������Ʈ ���ش�.
		if (mat->has_normal_texture)
		{
			glUniform1i(g_model.loc_is_use_tangent, true);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, mat->gl_normal);
		}
		else
		{
			glUniform1i(g_model.loc_is_use_tangent, false);
		}

		// ���������� VAO�� ���ε��ϰ�, mesh index ������ ���� �������Ѵ�.
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, (GLsizei)mesh.indices.size(), GL_UNSIGNED_INT, 0);
	}
}

struct ImguiGLBackEnd
{
	GLuint shader_vertex;
	GLuint shader_frag;
	GLuint pso_imgui; // PipelineStateObject
	GLuint vao_ui;
	GLuint vbo_ui;
	GLuint ibo_ui;
	GLuint tex_font;

	GLint loc_projection;
	GLint loc_texture;
} g_imgui_gl;

void imgui_init()
{
	// Create Imgui Context and set color scheme
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// link glfw input into imgui
	ImGuiIO& io = ImGui::GetIO();
	//io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
	//io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)

	// Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array.
	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
	io.SetClipboardTextFn = imgui_glfw_clipboard_set;
	io.GetClipboardTextFn = imgui_glfw_clipboard_get;
	io.ClipboardUserData = (void*)g_window;

	// glfw callback���� ����� input�� imgui�� ���޵ǰ� �Ѵ�.
	glfwSetScrollCallback(g_window, imgui_glfw_scroll_callback);
	glfwSetKeyCallback(g_window, imgui_glfw_key_callback);
	glfwSetCharCallback(g_window, imgui_glfw_char_callback);

	// initialize imgui opengl objects
	const char* vertex_shader =
		"#version 330 core\n"
		"layout (location = 0) in vec2 Position; "
		"layout (location = 1) in vec2 UV; "
		"layout (location = 2) in vec4 Color; "
		"uniform mat4 ProjMtx; "
		"out vec2 Frag_UV; "
		"out vec4 Frag_Color; "
		"void main() "
		"{ "
		"   Frag_UV = UV; "
		"   Frag_Color = Color; "
		"   gl_Position = ProjMtx * vec4(Position.xy, 0, 1); "
		"}";
	const char* frag_shader =
		"#version 330 core\n"
		"in vec2 Frag_UV; "
		"in vec4 Frag_Color; "
		"uniform sampler2D Texture; "
		"layout (location = 0) out vec4 Out_Color; "
		"void main() "
		"{ "
		"   Out_Color = Frag_Color * texture(Texture, Frag_UV.st); "
		"}";
	GLuint vso = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vso, 1, &vertex_shader, NULL);
	glCompileShader(vso);
	{
		GLint success;
		glGetShaderiv(vso, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			char info_logs[512];
			glGetShaderInfoLog(vso, 512, NULL, info_logs);
			printf("Fail to Compile source : %s\n", info_logs);
			assert(false);
		}
	}

	GLuint fso = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fso, 1, &frag_shader, NULL);
	glCompileShader(fso);
	{
		GLint success;
		glGetShaderiv(fso, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			char info_logs[512];
			glGetShaderInfoLog(fso, 512, NULL, info_logs);
			printf("Fail to Compile source : %s\n", info_logs);
			assert(false);
		}
	}

	GLuint pso = glCreateProgram();
	glAttachShader(pso, vso);
	glAttachShader(pso, fso);
	glLinkProgram(pso);
	{
		GLint success;
		glGetProgramiv(pso, GL_LINK_STATUS, &success);
		if (!success)
		{
			char info_logs[512];
			glGetProgramInfoLog(pso, 512, NULL, info_logs);
			printf("Fail to Compile source : %s\n", info_logs);
			assert(false);
		}
	}

	// Build texture atlas
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// ImGui�� ���� ���������� ����� font�� �����͸� ������ �����ϰ�, texture�� �ø���.
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	io.Fonts->SetTexID((ImTextureID)(intptr_t)tex);

	g_imgui_gl.shader_vertex = vso;
	g_imgui_gl.shader_frag = fso;
	g_imgui_gl.pso_imgui = pso;
	g_imgui_gl.tex_font = tex;

	// �ش� pso���� uniform ������ ��ġ�� �����;�, �ش� ��ġ�� �����͸� �־��� �� �ִ�.
	g_imgui_gl.loc_projection = glGetUniformLocation(pso, "ProjMtx");
	g_imgui_gl.loc_texture = glGetUniformLocation(pso, "Texture");

	// imgui �������� �ʿ��� ���� �غ�
	glGenBuffers(1, &(g_imgui_gl.vbo_ui));
	glGenBuffers(1, &(g_imgui_gl.ibo_ui));
	glGenVertexArrays(1, &(g_imgui_gl.vao_ui));
	glBindVertexArray(g_imgui_gl.vao_ui);
	{
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ARRAY_BUFFER, g_imgui_gl.vbo_ui);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void*)offsetof(ImDrawVert, pos));
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void*)offsetof(ImDrawVert, uv));
		glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void*)offsetof(ImDrawVert, col));
		// ImGui���� �ִ� Color���� �Ƹ� 0 ~ 255�� unsigned integer (byte, 8bit)���̱� ������, shader���� 0 ~ 1 ������
		// ������ �ٲپ�� �ϱ� ������, GL_TRUE�� Normalization�� ���� �ùٸ��� ������ �ǵ��� �Ѵ�.

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_imgui_gl.ibo_ui);
	}
}

void imgui_terminate()
{
	glDeleteVertexArrays(1, &(g_imgui_gl.vao_ui));
	glDeleteBuffers(1, &(g_imgui_gl.ibo_ui));
	glDeleteBuffers(1, &(g_imgui_gl.vbo_ui));
	glDeleteTextures(1, &(g_imgui_gl.tex_font));
	glDeleteProgram(g_imgui_gl.pso_imgui);
	glDeleteShader(g_imgui_gl.shader_frag);
	glDeleteShader(g_imgui_gl.shader_vertex);

	ImGui::DestroyContext();
}

void imgui_prepare()
{
	int w, h;
	int display_w, display_h;
	glfwGetWindowSize(g_window, &w, &h);
	glfwGetFramebufferSize(g_window, &display_w, &display_h);

	// �� �����Ӹ���, ImGui �����͸� ����� ����
	// Display ũ��, ���� �����Ӱ��� delta time, �׸��� input ���µ���
	// �ٽ� ������Ʈ ���־�� �Ѵ�.
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)w, (float)h);
	if (w > 0 && h > 0)
	{
		io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
	}

	// Set up time step
	double current_time = glfwGetTime();
	io.DeltaTime = g_time > 0.0 ? (float)(current_time - g_time) : (float)(1.0f / 60.0f);
	g_time = current_time;

	for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
	{
		// if a mouse press event came,
		// always pass it as "mouse held this frame",
		// so we don't miss click-release events that are shorter than 1 frame.
		io.MouseDown[i] = glfwGetMouseButton(g_window, i) != 0;
	}

	const ImVec2 mouse_pos_backup = io.MousePos;
	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

	const bool focused = glfwGetWindowAttrib(g_window, GLFW_FOCUSED) != 0;
	if (focused)
	{
		if (io.WantSetMousePos)
		{
			glfwSetCursorPos(g_window, (double)mouse_pos_backup.x, (double)mouse_pos_backup.y);
		}
		else
		{
			double mouse_x, mouse_y;
			glfwGetCursorPos(g_window, &mouse_x, &mouse_y);
			io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
		}
	}

	// Modifies are not reliable across systems
	io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
	io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
	io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
#ifdef _WIN32
	io.KeySuper = false;
#else
	io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
#endif
}

void imgui_draw()
{
	ImDrawData* draw_data = ImGui::GetDrawData();
	int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
	int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0)
		return;

	// imgui draw�� �ʿ��� rasterization ���·� ����
	// �ٸ� �ڵ�� rasterization�� �����õ� ���� ������ ���� �� �ֱ� ������
	// ���� ������ �����صд�.
	GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
	GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
	GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
	GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
	GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
	GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
	GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
	GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
	GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
	GLboolean last_enable_stencil_test = glIsEnabled(GL_STENCIL_TEST);
	GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
	GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);

	// ImGui�������� �ʿ��� rasterization state
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// 2D Rendering�̹Ƿ�, Orthogonal Projection�� ���
	float L = draw_data->DisplayPos.x;
	float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	float T = draw_data->DisplayPos.y;
	float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
	const float ortho_projection[4][4] =
	{
		{ 2.0f / (R - L),   0.0f,         0.0f,   0.0f },
		{ 0.0f,         2.0f / (T - B),   0.0f,   0.0f },
		{ 0.0f,         0.0f,        -1.0f,   0.0f },
		{ (R + L) / (L - R),  (T + B) / (B - T),  0.0f,   1.0f },
	};

	// imgui pso�� ����ϰ�, ���� �����͸� ������.
	glUseProgram(g_imgui_gl.pso_imgui);
	glUniform1i(g_imgui_gl.loc_texture, 0);
	glUniformMatrix4fv(g_imgui_gl.loc_projection, 1, GL_FALSE, &ortho_projection[0][0]);

	// ���� �غ�
	glBindVertexArray(g_imgui_gl.vao_ui);

	ImVec2 clip_off = draw_data->DisplayPos;		 // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// ������ ũ�Ⱑ �ٲ� ���� �����Ƿ� �׻� viewport�� ����
	glViewport(0, 0, fb_width, fb_height);

	// Render command lists
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];

		// Upload vertex/index buffers
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

			// Project scissor/clipping rectangles into framebuffer space
			ImVec4 clip_rect;
			clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
			clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
			clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
			clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

			if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
			{
				// Apply scissor/clipping rectangle
				glScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));

				// Bind texture, Draw
				glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID());
				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
			}
		}
	}

	// ������ rasterization state�� ������� �������´�.
	glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
	glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
	if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
	if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
	if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	if (last_enable_stencil_test) glEnable(GL_STENCIL_TEST); else glDisable(GL_STENCIL_TEST);
	if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
}

void imgui_glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseWheelH += (float)xoffset;
	io.MouseWheel += (float)yoffset;
}

void imgui_glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if (key >= 0 && key < IM_ARRAYSIZE(io.KeysDown))
	{
		if (action == GLFW_PRESS)
			io.KeysDown[key] = true;
		else if (action == GLFW_RELEASE)
			io.KeysDown[key] = false;
	}
}

void imgui_glfw_char_callback(GLFWwindow* window, unsigned int c)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharacter(c);
}

const char* imgui_glfw_clipboard_get(void* user_data)
{
	return glfwGetClipboardString((GLFWwindow*)user_data);
}

void imgui_glfw_clipboard_set(void* user_data, const char* text)
{
	glfwSetClipboardString((GLFWwindow*)user_data, text);
}

void glfw_init()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw�� ����� �ش� OS�� �´� window â�� �����ϰ�, �׿� ���õ� egl context�� ������.
	g_window = glfwCreateWindow(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, "Third Lecture", NULL, NULL);
	if (g_window == NULL)
	{
		printf("Failed to create GLFW Window\n");
		assert(false);
	}
	glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);
	glfwMakeContextCurrent(g_window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD");
		assert(false);
	}
}

void glfw_terminate()
{
	glfwTerminate();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void do_your_gui_code()
{
	// ImGui::ShowDemoWindow();
	if (ImGui::Begin("Information", 0, ImGuiWindowFlags_HorizontalScrollbar))
	{
		// User Interaction�� ���� ���� GUI �����ֱ�.

		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::Separator();

		ImGui::Text("Mouse Delta %f %f", io.MouseDelta.x, io.MouseDelta.y);
		ImGui::Text("Camera Position : %f %f %f", g_camera.position.x, g_camera.position.y, g_camera.position.z);
		ImGui::Text("Camera Right : %f %f %f", g_camera.right.x, g_camera.right.y, g_camera.right.z);
		ImGui::Text("Camera Up : %f %f %f", g_camera.up.x, g_camera.up.y, g_camera.up.z);
		ImGui::Text("Camera Forward : %f %f %f", g_camera.forward.x, g_camera.forward.y, g_camera.forward.z);
		ImGui::Text("Camera Mouse Sensitivity"); ImGui::SameLine();
		ImGui::DragFloat("##CameraMouseSensitivity", &(g_camera.mouse_sensitivity), 0.0001f, 0.01f, 0.1f, "%.4f");
		ImGui::Text("Camera Move Speed"); ImGui::SameLine();
		ImGui::DragFloat("##CameraMoveSpeed", &(g_camera.move_speed), 0.01f, 1.f, 100.f, "%.2f");
		if (ImGui::Button("Camera Reset"))
		{
			camera_reset();
		}

		ImGui::Separator();

		ImGui::Text("Model Position"); ImGui::SameLine();
		ImGui::DragFloat3("##ModelPosition", &g_model.position.x, 0.01f, FLT_MAX, -FLT_MAX, "%.2f");
		ImGui::Text("Model Rotation"); ImGui::SameLine();
		ImGui::DragFloat3("##ModelRotation", &g_model.rot_euler.x, 0.1f, FLT_MAX, -FLT_MAX, "%.1f");
		ImGui::Text("Model Scale"); ImGui::SameLine();
		ImGui::DragFloat3("##ModelScale", &g_model.scale.x, 0.001f, FLT_MAX, -FLT_MAX, "%.3f");
		if (ImGui::Button("Model Reset"))
		{
			g_model.scale = INITIAL_MODEL_SCALE;
			g_model.position = INITIAL_MODEL_POSITION;
			g_model.rot_euler = INITIAL_MODEL_ROTATION;
		}

		ImGui::Separator();

		ImGui::Text("Light Rotataion"); ImGui::SameLine();
		ImGui::DragFloat3("##LightRotataion", &g_light.rot_euler.x, 0.01f, FLT_MAX, -FLT_MAX, "%.2f");
		ImGui::Text("Light Ambient"); ImGui::SameLine();
		ImGui::ColorEdit3("##LightAmbient", &g_light.ambient.x);
		ImGui::Text("Light Diffuse"); ImGui::SameLine();
		ImGui::ColorEdit3("##LightDiffuse", &g_light.diffuse.x);
		ImGui::Text("Light Specular"); ImGui::SameLine();
		ImGui::ColorEdit3("##LightSpecular", &g_light.specular.x);
		if (ImGui::Button("Light Reset"))
		{
			g_light.rot_euler = INITIAL_LIGHT_ROT_EULER;
			g_light.ambient = INITIAL_LIGHT_AMBIENT;
			g_light.diffuse = INITIAL_LIGHT_DIFFUSE;
			g_light.specular = INITIAL_LIGHT_SPECULAR;
		}

		ImGui::Separator();

		ImGui::Text("Sort Draw Order"); ImGui::SameLine();
		ImGui::Checkbox("##SortDrawOrder", &is_sort_draw_order);

		ImGui::Separator();

		if (ImGui::TreeNode("Materials"))
		{
			for (int i = 0; i < g_model.material.size(); ++i)
			{
				Material& mat = g_model.material[i];

				ImGui::PushID(i);
				{
					if (ImGui::TreeNode(mat.debug_mat_name))
					{
						ImGui::Indent();
						{
							ImGui::Text("Material Ambient"); ImGui::SameLine();
							ImGui::ColorEdit3("##MaterialAmbient", &mat.ambient.x);

							ImGui::Text("Material Diffuse"); ImGui::SameLine();
							ImGui::ColorEdit3("##MaterialDiffuse", &mat.diffuse.x);

							ImGui::Text("Material Specular"); ImGui::SameLine();
							ImGui::ColorEdit3("##MaterialSpecular", &mat.specular.x);

							ImGui::Text("Material Shininess"); ImGui::SameLine();
							ImGui::DragFloat("##MaterialShininess", &(mat.shininess), 0.01f, 0.f, 100.f, "%.2f");

							ImGui::Text("TwoSided"); ImGui::SameLine();
							ImGui::Checkbox("##MaterialTwoSided", &mat.two_sided);

							ImGui::Text("Has Diffuse Map : %s", mat.gl_diffuse != g_default_texture_white ? "YES" : "NO");
							ImGui::Text("Has Normal Map : %s", mat.has_normal_texture ? "YES" : "NO");
							ImGui::Text("IsTransparent : %s", mat.is_transparent ? "YES" : "NO");
						}
						ImGui::Unindent();

						ImGui::TreePop();
					}
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}

		ImGui::Separator();
	}
	ImGui::End();
}