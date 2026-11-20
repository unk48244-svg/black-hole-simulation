#include "window.h"
#include <iostream>

namespace window
{
    GLFWwindow* createWindow()
    {
        if (!glfwInit())
        {
            std::cout << "GLFW initialization failed!\n";
            return nullptr;
        }

        GLFWwindow* window = glfwCreateWindow(
            900, 600,
            "Black Hole Simulation",
            nullptr, nullptr
        );

        if (!window)
        {
            glfwTerminate();
            return nullptr;
        }

        glfwMakeContextCurrent(window);

        return window;
    }
} 