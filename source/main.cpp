#include <stdio.h>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"

int main(void)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    constexpr int INITIAL_WINDOW_WIDTH = 800;
    constexpr int INITIAL_WINDOW_HEIGHT = 600;

    GLFWwindow* window = glfwCreateWindow(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, "LearnOpenGL", NULL, NULL);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD");
        return -1;
    }

    // check version
    const char* ver = (const char*)glGetString(GL_VERSION);
    printf("GL Version : %s\n", ver);

    // Vertex Shader
    const char* gl_vs =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;"
        "void main()"
        "{"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);"
        "}";
    unsigned vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &gl_vs, NULL);
    glCompileShader(vertex_shader);
    {
        GLint isSuccess;
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &isSuccess);
        if (!isSuccess)
        {
            char info_logs[512];
            glGetShaderInfoLog(vertex_shader, 512, NULL, info_logs);
            printf("Fail to Compile source : %s\n", info_logs);
            return -1;
        }
    }

    // Fragment Shader
    const char* gl_fs =
        "#version 330 core\n"
        "layout (location = 0) out vec4 fragColor;"
        "void main()"
        "{"
        "   fragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);"
        "};";
    unsigned frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader, 1, &gl_fs, NULL);
    glCompileShader(frag_shader);
    {
        GLint isSuccess;
        glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &isSuccess);
        if (!isSuccess)
        {
            char info_logs[512];
            glGetShaderInfoLog(frag_shader, 512, NULL, info_logs);
            printf("Fail to Compile source : %s\n", info_logs);
            return -1;
        }
    }

    // Pipeline Object 생성 (with vertex/fragment shader)
    unsigned shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, frag_shader);
    glLinkProgram(shader_program);
    {
        GLint isSuccess;
        glGetProgramiv(shader_program, GL_COMPILE_STATUS, &isSuccess);
        if (!isSuccess)
        {
            char info_logs[512];
            glGetProgramInfoLog(shader_program, 512, NULL, info_logs);
            printf("Fail to Create a PipelineState Object : %s\n", info_logs);
            return -1;
        }
    }

    // 삼각형 렌더링을 위한 Vertex Buffer
    float vertices[] =
    {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.05f
    };
    unsigned vbo;
    glGenBuffers(1, &vbo);

    // VAO에 어떤 Buffer를 어떻게 읽어들인건지 명시
    unsigned vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    {
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }


    glViewport(0, 0, INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT);
    
    glUseProgram(shader_program);
    glBindVertexArray(vao);

    while (!glfwWindowShouldClose(window))
    {
        // glfw 라이브러리에서 window와 관련된 input들을 처리
        glfwPollEvents();

        // 기존 framebuffer의 color data를 초기화
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

        // color buffer bit 초기화
        glClear(GL_COLOR_BUFFER_BIT);

        // draw call
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // 화면에 그려지도록 요청
        glfwSwapBuffers(window);
    }

    // 메모리 해제
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader_program);
    glDeleteShader(frag_shader);
    glDeleteShader(vertex_shader);

    glfwTerminate();

    return 0;
}