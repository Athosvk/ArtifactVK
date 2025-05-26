#pragma once
#include "VulkanInstance.h"

#include <string>
#include <unordered_map>
#include <optional>

struct WindowCreateInfo
{
    uint32_t Width;
    uint32_t Height;
    std::string Name;
};

struct WindowResizeEvent
{
    uint32_t NewWidth;
    uint32_t NewHeight;
};

struct GLFWwindow;

class Window
{
  public:
    Window(const WindowCreateInfo &windowParams);
    ~Window();
    bool ShouldClose() const;
    std::optional<WindowResizeEvent> PollEvents();
    VulkanInstance CreateVulkanInstance(const InstanceCreateInfo &createInfo);
  private:
    void OnWindowResize(WindowResizeEvent resizeEvent);
     
    GLFWwindow *m_InternalWindow;
    std::optional<WindowResizeEvent> m_LastWindowResizeEvent;
};
