#pragma once
#include <vulkan/vulkan.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "backend/VulkanInstance.h"
#include "backend/Window.h"
#include "backend/Pipeline.h"

class App
{
  public:
    App();
    ~App();

    void RunRenderLoop();

  private:
    Window m_Window;
    VulkanInstance m_VulkanInstance;
    Pipeline m_Pipeline;
};
