#include "App.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

#include "backend/ShaderModule.h"

const InstanceCreateInfo DefaultCreateInfo()
{
    InstanceCreateInfo createInfo;
    createInfo.Name = "ArtifactVK";
    createInfo.ValidationLayers =
        std::vector<ValidationLayer>{ValidationLayer{EValidationLayer::KhronosValidation, false}};
    createInfo.RequiredExtensions = std::vector<EDeviceExtension>{EDeviceExtension::VkSwapchain};
    return createInfo;
}

App::App()
    : m_Window(WindowCreateInfo{800, 600, "ArtifactVK"}),
      m_VulkanInstance(m_Window.CreateVulkanInstance(DefaultCreateInfo())),
      m_MainPass(m_VulkanInstance.GetActiveDevice().CreateRenderPass()),
      m_RenderFullscreen(LoadShaderPipeline(m_VulkanInstance.GetActiveDevice(), m_MainPass))
{
}

App::~App()
{
    glfwTerminate();
}

void App::RunRenderLoop()
{
    while (!m_Window.ShouldClose())
    {
        m_Window.PollEvents();
    }
}

RasterPipeline App::LoadShaderPipeline(LogicalVulkanDevice &vulkanDevice, const RenderPass& renderPass) const
{
    return vulkanDevice.CreateRasterPipeline(
        RasterPipelineBuilder("spirv/triangle.vert.spv", "spirv/triangle.frag.spv"));
}
