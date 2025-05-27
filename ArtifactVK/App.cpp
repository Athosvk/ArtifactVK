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
      m_RenderFullscreen(LoadShaderPipeline(m_VulkanInstance.GetActiveDevice(), m_MainPass)),
      m_SwapchainFramebuffers(m_VulkanInstance.GetActiveDevice().CreateSwapchainFramebuffers(m_MainPass)),
      m_PerFrameState(CreatePerFrameState(m_VulkanInstance.GetActiveDevice())),
      m_Swapchain(m_VulkanInstance.GetActiveDevice().GetSwapchain())
{
}

App::~App()
{
    for (auto &perFrameState : m_PerFrameState)
    {
        perFrameState.CommandBuffer.WaitFence(true);
    }
    glfwTerminate();
}

void App::RunRenderLoop()
{
    while (!m_Window.ShouldClose())
    {
        if (!m_Window.IsMinimized())
        {
            std::cout << "\nRendering frame " << m_CurrentFrameIndex << "\n";
            auto resizeEvent = m_Window.PollEvents();
            if (resizeEvent.has_value() && !m_Window.IsMinimized())
            {
                m_VulkanInstance.GetActiveDevice().HandleResizeEvent(*resizeEvent);
            }
            
            if (m_Window.IsMinimized())
            {
                m_Window.WaitForRender();
            }
            else
            {
				RecordFrame(m_PerFrameState[m_CurrentFrameIndex % 2]);
				m_CurrentFrameIndex += 1;
            }

        }
    }
}

RasterPipeline App::LoadShaderPipeline(LogicalVulkanDevice &vulkanDevice, const RenderPass& renderPass) const
{
    return vulkanDevice.CreateRasterPipeline(
        RasterPipelineBuilder("spirv/triangle.vert.spv", "spirv/triangle.frag.spv"), renderPass);
}

void App::RecordFrame(PerFrameState& state)
{
    m_VulkanInstance.GetActiveDevice().AcquireNext(state.ImageAvailable);
    // TODO: Can probably be moved to CommandBuffer->Begin()
    state.CommandBuffer.WaitFence();
    state.CommandBuffer.Begin();
    state.CommandBuffer.Draw(m_SwapchainFramebuffers.GetCurrent(), m_MainPass, m_RenderFullscreen);
    state.CommandBuffer.End(std::span{ &state.ImageAvailable, 1 }, std::span{ &state.RenderFinished, 1 }, 
        m_VulkanInstance.GetActiveDevice().GetGraphicsQueue());
    
    m_VulkanInstance.GetActiveDevice().Present(std::span{&state.RenderFinished, 1});
}

std::vector<std::reference_wrapper<Semaphore>> App::CreateSemaphorePerInFlightFrame()
{
    std::vector<std::reference_wrapper<Semaphore>> semaphores;
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        semaphores.emplace_back(m_VulkanInstance.GetActiveDevice().CreateDeviceSemaphore());
    }
    return semaphores;
}

std::vector<PerFrameState> App::CreatePerFrameState(LogicalVulkanDevice &vulkanDevice)
{
    std::vector<PerFrameState> perFrameState;
    auto commandBuffers = vulkanDevice.CreateGraphicsCommandBufferPool().CreateCommandBuffers(MAX_FRAMES_IN_FLIGHT);
    perFrameState.reserve(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        perFrameState.emplace_back(PerFrameState{vulkanDevice.CreateDeviceSemaphore(),
                                                 vulkanDevice.CreateDeviceSemaphore(), commandBuffers[i]});
    }
    return perFrameState;
}

constexpr std::vector<Vertex> App::GetVertices()
{
    return
    {
	    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };
}
