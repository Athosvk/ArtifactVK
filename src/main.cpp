// ArtifactVK.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <iostream>

#include "App.h"

#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

int main()
{
    // TODO: Move to app init?
    glfwInit();
    {
        std::vector<int> i;
        std::vector<int> b = std::move(i);
    }
    App app;
    app.RunRenderLoop();
    return 0;
}
