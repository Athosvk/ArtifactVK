#include "Window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>

Window::Window(const WindowCreateInfo &windowParams)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_InternalWindow =
        glfwCreateWindow(windowParams.Width, windowParams.Height, windowParams.Name.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(m_InternalWindow, this);
    glfwSetFramebufferSizeCallback(m_InternalWindow, [](auto window, int width, int height) { 
        reinterpret_cast<Window*>(glfwGetWindowUserPointer(window))->OnWindowResize(WindowResizeEvent{
            static_cast<uint32_t>(width), 
            static_cast<uint32_t>(height) 
        });
    });
}

Window::~Window()
{
    glfwDestroyWindow(m_InternalWindow);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_InternalWindow);
}

std::optional<WindowResizeEvent> Window::PollEvents()
{
    glfwPollEvents();
    if (m_LastWindowResizeEvent)
    {
        auto resizeEvent = *std::move(m_LastWindowResizeEvent);
        m_LastWindowResizeEvent.reset();
        return resizeEvent;
    }
    else
    {
        return std::nullopt;
    }

}

VulkanInstance Window::CreateVulkanInstance(const InstanceCreateInfo &createInfo)
{
    return VulkanInstance(createInfo, *m_InternalWindow);
}

void Window::OnWindowResize(WindowResizeEvent resizeEvent)
{
    m_LastWindowResizeEvent = resizeEvent;

    std::cout << "Resize event size: " << resizeEvent.NewWidth << "," << resizeEvent.NewHeight
              << "\n";
}
