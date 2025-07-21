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
#include "backend/DescriptorSetBuilder.h"
#include "Image.h"

class VertexBuffer;
class IndexBuffer;
class UniformBuffer;
class DescriptorSetLayout;

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct PerFrameState
{
    Semaphore &ImageAvailable;
    Semaphore &RenderFinished;
    CommandBuffer &CommandBuffer;
    UniformBuffer &UniformBuffer;
    DescriptorSet DescriptorSet;
};

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec2 UV;

    constexpr static VkVertexInputBindingDescription GetBindingDescription();
    constexpr static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions(); 
    constexpr static VertexBindingDescription GetVertexBindingDescription();
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
    Texture& LoadImage();
    UniformConstants GetUniforms();
    RasterPipeline LoadShaderPipeline(VulkanDevice &vulkanDevice, const RenderPass& renderPass) const;
    void RecordFrame(PerFrameState& state);
    std::vector<std::reference_wrapper<Semaphore>> CreateSemaphorePerInFlightFrame();
    std::vector<PerFrameState> CreatePerFrameState(VulkanDevice &vulkanDevice);
    constexpr static std::vector<Vertex> GetVertices();
    constexpr static std::vector<uint16_t> GetIndices();
    const DescriptorSetLayout& BuildDescriptorSetLayout(VulkanDevice &vulkanDevice) const;

    Window m_Window;
    VulkanInstance m_VulkanInstance;
    RenderPass m_MainPass;
    const SwapchainFramebuffer& m_SwapchainFramebuffers;
    const DescriptorSetLayout &m_DescriptorSetLayout;
    std::vector<PerFrameState> m_PerFrameState;
    RasterPipeline m_RenderFullscreen;
    uint32_t m_CurrentFrameIndex = 0;
    Swapchain &m_Swapchain;
    VertexBuffer &m_VertexBuffer;
    IndexBuffer &m_IndexBuffer;
    Texture& m_Texture;
};
