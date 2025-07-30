#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "pbf_solver.h" 

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

int main(int argc, char* argv[]) {
    // 1. 初始化 GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. 创建窗口
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PBF Fluid Simulation", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 3. 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    RenderMode renderMode = RenderMode::PARTICLES; // 默认是粒子模式
    if (argc > 1 && std::string(argv[1]) == "fluid") {
        renderMode = RenderMode::FLUID;
        std::cout << "Render mode set to: FLUID" << std::endl;
    } else {
        std::cout << "Render mode set to: PARTICLES" << std::endl;
        std::cout << "Hint: Run with './build/pbf_sim fluid' to enable fluid rendering." << std::endl;
    }

    glEnable(GL_DEPTH_TEST);
    // 启用 gl_PointSize，让顶点着色器可以控制点的大小
    glEnable(GL_PROGRAM_POINT_SIZE);

    // 4. 创建我们的模拟器实例
    PBFSolver solver;
    solver.initialize(renderMode);

    // 5. 渲染循环 (Game Loop)
    while (!glfwWindowShouldClose(window)) {
        // 输入
        processInput(window);

        // 更新
        float dt = 0.016f; // 固定时间步长
        solver.update(dt);

        // 渲染
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 在这里可以设置简单的 MVP 矩阵等，但为了简单，暂时省略
        // 简单设置点的大小
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        glm::vec3 cameraPos = glm::vec3(0.0f, 4.0f, 12.0f);
        // 2. View 矩阵 (相机)
        // 将相机放在 (0, 2, 8) 的位置，看向原点 (0, 1, 0)，头部朝上 (0, 1, 0)
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 2.0f, 8.0f),
                                glm::vec3(0.0f, 1.0f, 0.0f),
                                glm::vec3(0.0f, 1.0f, 0.0f));

        // 调用新的 render 函数，并传入矩阵
        solver.render(projection, view, cameraPos);

        // 交换缓冲区和处理事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// ‼️ 新增: processInput 函数的定义
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// ‼️ 新增: framebuffer_size_callback 函数的定义
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}