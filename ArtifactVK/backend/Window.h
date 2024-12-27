#pragma once
#include "VulkanInstance.h"
#include <string>

struct WindowCreateInfo
{
    uint32_t Width;
    uint32_t Height;
    std::string Name;
};

struct GLFWwindow;

class Window
{
  public:
    Window(const WindowCreateInfo &windowParams);
    ~Window();
    bool ShouldClose() const;
    void PollEvents() const;
    VulkanInstance CreateVulkanInstance(const InstanceCreateInfo &createInfo);

  private:
    GLFWwindow *m_InternalWindow;
};
