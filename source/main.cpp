#include <stdio.h>
#include "glad/glad.h"
#include "GLFW/glfw3.h"

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

    const char* ver = (const char*)glGetString(GL_VERSION);
    printf("GL Version : %s\n", ver);

    return 0;
}