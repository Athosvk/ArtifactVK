#include <backend/Window.h>

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

Window::Window(Window &&other) : m_InternalWindow(std::exchange(other.m_InternalWindow, nullptr)), m_LastWindowResizeEvent(other.m_LastWindowResizeEvent)
{
}


Window::~Window()
{
    if (m_InternalWindow != nullptr) 
    {
        glfwDestroyWindow(m_InternalWindow);
    }
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

bool Window::IsMinimized() const
{
    return m_Minimized;
}

void Window::WaitForRender()
{
    while (m_Minimized)
    {
        int width, height;
        glfwGetFramebufferSize(m_InternalWindow, &width, &height);
        glfwWaitEvents();
    }
}

void Window::OnWindowResize(WindowResizeEvent resizeEvent)
{
    if (resizeEvent.NewWidth == 0 || resizeEvent.NewHeight == 0)
    {
        m_Minimized = true;
    } 
    else 
    {
        m_Minimized = false;
    }
    m_LastWindowResizeEvent = resizeEvent;
}
