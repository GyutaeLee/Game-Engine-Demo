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

	while (!glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		// ImGui Data 준비
		{
			imgui_prepare();		// ImGui의 새로운 프레임을 위한 display,time,interaction 정보 설정
			ImGui::NewFrame();		// 새로운 프레임을 위한 ImGui 상태 정리
			{
				do_your_gui_code(); // ImGui 코드 작성
			}
			ImGui::Render();		// ImGui 렌더링 데이터 형성
		}

		// ImGui Data 렌더링
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		imgui_draw();					// ImGUi의 렌더링 데이터를 GPU에 올려서 처리한다.

		glfwSwapBuffers(g_window);
	}

	imgui_terminate();
	glfw_terminate();

	return 0;
}

void do_your_gui_code()
{
	ImGui::ShowDemoWindow();

	if (ImGui::Begin("MyWindow"))
	{
		static bool show_text = false;
		if (ImGui::Button("First Button") == true)
		{
			show_text = !show_text;
		}

		if (show_text)
		{
			ImGui::Text("You Clicked!");
		}
	}
	ImGui::End();
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
	// 임의로 move speed와 mouse sensitivity를 적당하게 설정
	g_camera.move_speed = 50.0f;
	g_camera.mouse_sensitivity = 0.07f;

	// 카메라 위치와 회전값도 마찬가지.
	g_camera.position = glm::vec3(0.0f, 5.0f, 10.0f);
	g_camera.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	// rotation (1, 0, 0, 0)이 그대로 정면을 바라보고 있으므로, right/up/forward는 3차원 공간에서의 기저와 같다.
	g_camera.right = glm::vec3(1.0f, 0.0f, 0.0f);
	g_camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_camera.forward = glm::vec3(0.0f, 0.0f, 1.0f);

	// 설정된 rotation과 position에 따라 view matrix를 갱신
	g_camera.view = glm::mat4_cast(glm::conjugate(g_camera.rotation));
	g_camera.view[3][0] = -(g_camera.view[0][0] * g_camera.position.x + g_camera.view[1][0] * g_camera.position.y + g_camera.view[2][0] * g_camera.position.z);
	g_camera.view[3][1] = -(g_camera.view[0][1] * g_camera.position.x + g_camera.view[1][1] * g_camera.position.y + g_camera.view[2][1] * g_camera.position.z);
	g_camera.view[3][2] = -(g_camera.view[0][2] * g_camera.position.x + g_camera.view[1][2] * g_camera.position.y + g_camera.view[2][2] * g_camera.position.z);

	// perspective projection을 위해 fov와 near/far plane 값을 설정하고, projection matrix를 갱신.
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
	// Window와의 Input Data는 glfw로도 가져올 수 있지만,
	// 우리는 그 데이터를 ImGui에 다 넣어주고 있기 때문에
	// 그냥 편하게 ImGui를 사용하도록 한다.
	// 원하면 glfw로도 구현할 수 있다.
	ImGuiIO& io = ImGui::GetIO();

	bool should_update_view_matrix = false;

	// 현재 마우스 왼쪽이 클릭되어있는지 && 그리고 ImGui UI가 클릭이 안되어 있는지
	// io.WantCaptureMouse에 대한 정보는 주석 참조할 것.
	if (io.MouseDown[GLFW_MOUSE_BUTTON_LEFT] && !io.WantCaptureMouse)
	{
		should_update_view_matrix = true;

		// mouse움직임의 delta값을 degree라고 가정하고, radian값으로 바꾸고 mouse sensitivitiy값을 곱하여
		// 원하는 민감도로 움직이게 한다.
		float x_delta = glm::radians(io.MouseDelta.x) * g_camera.mouse_sensitivity;
		float y_delta = glm::radians(io.MouseDelta.y) * g_camera.mouse_sensitivity;

		// 사용자가 x축으로 마우스를 움직였다면 y축에 대한 회전을 만들어야 한다.
		// 이 때 오른쪽으로 움직였다면 오른쪽으로 회전 시켜야 하는데, x_delta가 양수인데, 3차원 회전에서 양수의 값으로 회전시키면
		// 왼쪽으로 회전하므로 음수값으로 바꾸어준다.
		glm::quat y_rot = glm::angleAxis(-x_delta, glm::vec3(0.f, 1.f, 0.f));

		// 사용자가 y축으로 마우스를 움직였다면 x축에 대한 회전을 만들어야 한다.
		glm::quat x_rot = glm::angleAxis(-y_delta, glm::vec3(1.f, 0.0, 0.f));

		/*
			Rotation Order : Y -> Previous Rotation -> X
			Previous Rotation is also Y -> X
			결과적으로 전체 순서는 Y -> Y -> X -> X 이므로, 최종적으로 Y -> X이다.

			그래서 이전 회전에 대해 delta roation을 곱하는게 가능해진다.

			다음의 수식들을 직접 실행해봄으로써 위에서 말하는 것을, 특히 Y -> Y가 그 회전축에 대한 회전을 축적시키는지를 볼 수 있다
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

		// mouse 움직임 delta 값에 따라 카메라의 새로운 회전 갱신
		g_camera.rotation = (y_rot * g_camera.rotation) * x_rot;

		// 회전이 갱신되었으므로, quat을 matrix형태로 바꾸어 해당 좌표계의 기저들을 right/up/forward에 잘 넣어준다.
		glm::mat3 rot_mat = glm::mat3_cast(g_camera.rotation);
		g_camera.right = rot_mat[0];
		g_camera.up = rot_mat[1];
		g_camera.forward = rot_mat[2];
	}

	// 사용자가 ImGUi에 키보드 상호작용을 하고 있지 않을 때, 자세한 것은 주석 참조.
	if (!io.WantCaptureKeyboard)
	{
		// 이번 프레임의 delta time에 대해 move speed를 곱해 원하는 속도로 움직여주게 해준다.
		// delta time은 어떤 프레임에서든지 일정한 속도를 내게 해줄 것이다.
		float delta_move = io.DeltaTime * g_camera.move_speed;

		// 사용자가 shift키를 눌렀다면 10를 곱해 움직임 속도를 가속시켜준다.
		if (io.KeyShift)
		{
			delta_move *= 10.f;
		}

		// 사용자가 누른 키보드에 따라 일반적인 FPS 카메라의 움직임을 만들어준다.
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

	// 만약 camera의 position/rotation 둘 중 하나라도 업데이트 되었다면
	// view matrix를 업데이트 해준다.
	if (should_update_view_matrix)
	{
		// camera가 회전한 것과 역방향으로 모든 다른 사물들을 움직여야 하기 때문에
		// quaternion conjugate로 반대방향으로 회전시켜준다.
		// 그리고 camera position을 그 rotation 좌표계를 통해 view[3] position자리에 넣어준다.
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

	// perspective를 갱신해준다. 이것은 관련 값이 바뀔 때만 업데이트 해줘도 되는데
	// window dimension 갱신되는 것을 변수를 두고 확인하기가 귀찮아서 그냥 이렇게 매 프레임마다 갱신하게 둔다.
	g_camera.projection = glm::perspective(glm::radians(g_camera.fov_degree), aspect, g_camera.near_plane, g_camera.far_plane);
}

void process_scene_mesh(const aiScene* scene);
void process_scene_material(const aiScene* scene, const char* base_folder);

void model_init()
{
#if defined(_WIN32) || defined(_WIN64)
	// model불러오기전에 현재 이 exe가 어디 경로를 기반으로 상대경로를 불러올 수 있는지
	// 확인하기 위해 체크
	char* cwd = _getcwd(NULL, 0);
	printf("Current Dir : %s\n", cwd);
	free(cwd);
#endif
	// default white texture 생성
	unsigned char white[4] = { 255,255,255,255 };
	//glGenTextures(1, &g_default_texture_white));

}

void model_terminate();
void model_draw();

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

	// glfw callback으로 사용자 input을 imgui에 전달되게 한다.
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

	// ImGui의 글자 렌더링에서 사용할 font의 데이터를 위에서 형성하고, texture로 올린다.
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

	// 해당 pso에서 uniform 변수의 위치를 가져와야, 해당 위치에 데이터를 넣어줄 수 있다.
	g_imgui_gl.loc_projection = glGetUniformLocation(pso, "ProjMtx");
	g_imgui_gl.loc_texture = glGetUniformLocation(pso, "Texture");

	// imgui 렌더링에 필요한 버퍼 준비
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
		// ImGui에서 주는 Color값이 아마 0 ~ 255의 unsigned integer (byte, 8bit)값이기 때문에, shader에서 0 ~ 1 사이의
		// 값으로 바꾸어야 하기 때문에, GL_TRUE로 Normalization을 통해 올바르게 렌더링 되도록 한다.

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

	// 매 프레임마다, ImGui 데이터를 만들기 전에
	// Display 크기, 이전 프레임과의 delta time, 그리고 input 상태들을
	// 다시 업데이트 해주어야 한다.
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

	// imgui draw에 필요한 rasterization 상태로 인해
	// 다른 코드와 rasterization고 ㅏ관련된 상태 정보가 섞일 수 있기 때문에
	// 이전 정보를 저장해둔다.
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

	// ImGui렌더링에 필요한 rasterization state
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// 2D Rendering이므로, Orthogonal Projection을 사용
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

	// imgui pso를 사용하고, 관련 데이터를 보낸다.
	glUseProgram(g_imgui_gl.pso_imgui);
	glUniform1i(g_imgui_gl.loc_texture, 0);
	glUniformMatrix4fv(g_imgui_gl.loc_projection, 1, GL_FALSE, &ortho_projection[0][0]);

	// 버퍼 준비
	glBindVertexArray(g_imgui_gl.vao_ui);

	ImVec2 clip_off = draw_data->DisplayPos;		 // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// 윈도우 크기가 바뀔 수도 있으므로 항상 viewport를 설정
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

	// 이전의 rasterization state를 원래대로 돌려놓는다.
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

	// glfw를 사용해 해당 OS에 맞는 window 창을 형성하고, 그에 관련된 egl context를 형성함.
	g_window = glfwCreateWindow(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, "Second Week", NULL, NULL);
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