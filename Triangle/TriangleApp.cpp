#include "TriangleApp.h"
#include <GLFW/glfw3.h>

void TriangleApp::Run() {
    InitWindow();
    InitVulkan();
    Loop();
    CleanUp();
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::InitWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", nullptr, nullptr);

}

//---------------------------------------------------------------------------------------------------------------------
void TriangleApp::InitVulkan() {
}


//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::Loop() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void TriangleApp::CleanUp() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}
