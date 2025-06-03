#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <array>

#include "backend/VulkanInstance.h"
#include "backend/Window.h"
#include "backend/Pipeline.h"
#include "backend/RenderPass.h"
#include "backend/Swapchain.h"

class VertexBuffer;
class IndexBuffer;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct PerFrameState
{
    Semaphore &ImageAvailable;
    Semaphore &RenderFinished;
    CommandBuffer &CommandBuffer;
};

struct Vertex
{
    glm::vec2 Position;
    glm::vec3 Color;

    constexpr static VkVertexInputBindingDescription GetBindingDescription();
    constexpr static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions(); 
    constexpr static VertexBindingDescription GetVertexBindingDescription();
};

class App
{
  public:
    App();
    ~App();

    void RunRenderLoop();

  private:
    RasterPipeline LoadShaderPipeline(VulkanDevice &vulkanDevice, const RenderPass& renderPass) const;
    void RecordFrame(PerFrameState& state);
    std::vector<std::reference_wrapper<Semaphore>> CreateSemaphorePerInFlightFrame();
    std::vector<PerFrameState> CreatePerFrameState(VulkanDevice &vulkanDevice);
    constexpr static std::vector<Vertex> GetVertices();

    Window m_Window;
    VulkanInstance m_VulkanInstance;
    RenderPass m_MainPass;
    RasterPipeline m_RenderFullscreen;
    const SwapchainFramebuffer& m_SwapchainFramebuffers;
    std::vector<PerFrameState> m_PerFrameState;
    uint32_t m_CurrentFrameIndex = 0;
    Swapchain &m_Swapchain;
    VertexBuffer &m_VertexBuffer;
    IndexBuffer &m_IndexBuffer;
};
