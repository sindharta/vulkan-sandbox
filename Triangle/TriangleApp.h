#pragma once

#include <stdint.h>

struct GLFWwindow;

class TriangleApp {
public:
    void Run();

private:
    void InitWindow();
    void InitVulkan();
    void Loop(); 
    void CleanUp();

    GLFWwindow* m_window;

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
};