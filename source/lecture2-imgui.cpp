#include <stdio.h>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "imgui/imgui.h"

GLFWwindow* g_window = NULL;
double g_time = 0.0;

void do_your_gui_code();

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
	glfw_init();
	imgui_init();

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

		// ImGui Data ������
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		imgui_draw();					// ImGUi�� ������ �����͸� GPU�� �÷��� ó���Ѵ�.

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
	constexpr int INITIAL_WINDOW_WIDTH = 1250;
	constexpr int INITIAL_WINDOW_HEIGHT = 700;

	// glfw�� ����� �ش� OS�� �´� window â�� �����ϰ�, �׿� ���õ� egl context�� ������.
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