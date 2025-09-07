#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <functional>

#include <array>

#include <backend/VulkanInstance.h>
#include <backend/Window.h>
#include <backend/Pipeline.h>
#include <backend/RenderPass.h>
#include <backend/Swapchain.h>
#include <backend/DescriptorSetBuilder.h>

#include <Image.h>
#include <Vertex.h>
#include <Model.h>

class VertexBuffer;
class IndexBuffer;
class UniformBuffer;
class DescriptorSetLayout;
class Texture2D;
class DepthAttachment;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct PerFrameState
{
    Semaphore &ImageAvailable;
    Semaphore &RenderFinished;
    CommandBuffer &CommandBuffer;
    UniformBuffer &UniformBuffer;
    DescriptorSet DescriptorSet;
    TimerPool& TimerPool;
};

struct UniformConstants {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

class App
{
  public:
    App();
    ~App();

    void RunRenderLoop();

  private:
    Texture2D& LoadImage();
    Model LoadModel();
    DepthAttachment& CreateSwapchainDepthAttachment();
    UniformConstants GetUniforms();
    RasterPipeline LoadShaderPipeline(VulkanDevice &vulkanDevice, const RenderPass& renderPass) const;
    void RecordFrame(PerFrameState& state);
    std::vector<std::reference_wrapper<Semaphore>> CreateSemaphorePerInFlightFrame();
    std::vector<PerFrameState> CreatePerFrameState(VulkanDevice &vulkanDevice);
    std::vector<Vertex> GetVertices() const;
    std::vector<uint32_t> GetIndices() const;
    const DescriptorSetLayout& BuildDescriptorSetLayout(VulkanDevice &vulkanDevice) const;

    Model m_Model;
    Window m_Window;
    VulkanInstance m_VulkanInstance;
    DepthAttachment &m_DepthAttachment;
    RenderPass m_MainPass;
    const SwapchainFramebuffer& m_SwapchainFramebuffers;
    const DescriptorSetLayout &m_DescriptorSetLayout;
    std::vector<PerFrameState> m_PerFrameState;
    RasterPipeline m_RenderFullscreen;
    uint32_t m_CurrentFrameIndex = 0;
    Swapchain &m_Swapchain;
    VertexBuffer &m_VertexBuffer;
    IndexBuffer &m_IndexBuffer;
    Texture2D& m_Texture;
};
