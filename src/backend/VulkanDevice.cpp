#include <backend/VulkanDevice.h>

#include <cassert>
#include <condition_variable>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <limits>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <utility>
#include <array>
#include <iterator>
#include <algorithm>
#include <iterator>

#include <GLFW/glfw3.h>

#include <backend/VulkanSurface.h>
#include <backend/Window.h>
#include <backend/ShaderModule.h>
#include <backend/PhysicalDevice.h>
#include <backend/IndexBuffer.h>

VkSurfaceFormatKHR VulkanDevice::SelectSurfaceFormat() const
{
    auto surfaceProperties = m_PhysicalDevice.GetCachedSurfaceProperties();
    auto iter = std::find_if(surfaceProperties.Formats.begin(), surfaceProperties.Formats.end(), [](const VkSurfaceFormatKHR& format)
        {
        return format.colorSpace == VkColorSpaceKHR::VK_COLORSPACE_SRGB_NONLINEAR_KHR && format.format ==
            VkFormat::VK_FORMAT_B8G8R8A8_SRGB; 
        });
    if (iter != surfaceProperties.Formats.end())
    {
        return *iter;
    } 
    else
    {
        // Prefer the one above, but return something in case that fails.
        // TODO: Better surface format selection
        return surfaceProperties.Formats.front();
    }
}

VkPresentModeKHR VulkanDevice::SelectPresentMode() const
{
    auto surfaceProperties = m_PhysicalDevice.GetCachedSurfaceProperties();
    return std::find(surfaceProperties.PresentModes.begin(), surfaceProperties.PresentModes.end(), VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR) != surfaceProperties.PresentModes.end() ?
        VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR : VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanDevice::SelectSwapchainExtent(GLFWwindow& window, const SurfaceProperties& surfaceProperties) const
{
    if (surfaceProperties.Capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max())
    {
        return surfaceProperties.Capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(&window, &width, &height);
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
        };
        
        actualExtent.width = std::clamp(actualExtent.width, surfaceProperties.Capabilities.minImageExtent.width, surfaceProperties.Capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, surfaceProperties.Capabilities.minImageExtent.height, surfaceProperties.Capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

DeviceBuffer &VulkanDevice::CreateBuffer(const CreateBufferInfo &createInfo)
{
    return *m_Buffers.emplace_back(std::make_unique<DeviceBuffer>(m_Device, m_PhysicalDevice, createInfo));
}

Texture2D &VulkanDevice::CreateTexture(const Texture2DCreateInfo &createInfo)
{
    auto &commandBuffer = GetTransferCommandBuffer();
    // TODO: Embed texture name
    commandBuffer.SetName("Transfer Texture Command Buffer", GetExtensionFunctionMapping());
    return *m_Textures.emplace_back(std::make_unique<Texture2D>(m_Device, m_PhysicalDevice, createInfo, commandBuffer, 
        // TODO: Should also allow transferring to compute
        *m_GraphicsQueue));
}

DepthAttachment &VulkanDevice::CreateSapchainDepthAttachment()
{
    assert(m_GraphicsQueue && "No suitable graphics queue");

    DepthAttachmentCreateInfo createInfo{.Width = static_cast<uint32_t>(m_Swapchain->GetViewportDescription().Viewport.width),
                                         .Height = static_cast<uint32_t>(m_Swapchain->GetViewportDescription().Viewport.height)};
    return *m_DepthAttachments.emplace_back(
        std::make_unique<DepthAttachment>(m_Device, m_PhysicalDevice, createInfo, 
            // TODO: Don't just assume first is good here
            m_GraphicsCommandBufferPool->CreateCommandBuffer(*m_GraphicsQueue)));
}

DescriptorSet VulkanDevice::CreateDescriptorSet(const DescriptorSetLayout& layout)
{
    return m_DescriptorPool->CreateDescriptorSet(layout);
}

const DescriptorSetLayout& VulkanDevice::CreateDescriptorSetLayout(DescriptorSetBuilder builder)
{
    return *m_DescriptorSetLayouts.emplace_back(std::make_unique<DescriptorSetLayout>(builder.Build(m_Device)));
}

void VulkanDevice::WaitForIdle() const
{
    vkDeviceWaitIdle(m_Device);
}

Swapchain &VulkanDevice::CreateSwapchain(GLFWwindow &window, const VulkanSurface &surface)
{
    auto maxImageCount = m_PhysicalDevice.GetCachedSurfaceProperties().Capabilities.maxImageCount == 0 ? std::numeric_limits<uint32_t>::max()
                             : m_PhysicalDevice.GetCachedSurfaceProperties().Capabilities.maxImageCount;
    SwapchainCreateInfo createInfo{
        SelectSurfaceFormat(),
        SelectPresentMode(),
        SelectSwapchainExtent(window, m_PhysicalDevice.GetCachedSurfaceProperties()),
        // Select min image count + 1 if available
        std::min(m_PhysicalDevice.GetCachedSurfaceProperties().Capabilities.minImageCount + 1,
            maxImageCount)
    };
       
    assert(m_PresentQueue.has_value() && "Device has no present queue to present swapchain to");
    return m_Swapchain.emplace(Swapchain(createInfo, surface.Get(), m_Device, m_PhysicalDevice, m_PresentQueue.value()));
}

Swapchain &VulkanDevice::GetSwapchain()
{
    assert(m_Swapchain.has_value() && "Need an active swapchain. Create one through CreateSwapchain");
    return *m_Swapchain;
}

void VulkanDevice::RecreateSwapchain(VkExtent2D newSize)
{
    // TODO: Remove this through proper syncing with old swapchain
    WaitForIdle();

    for (auto& depthAttachment : m_DepthAttachments)
    {
        DepthAttachmentCreateInfo createInfo{.Width = newSize.width, .Height = newSize.height};

        *depthAttachment = DepthAttachment{m_Device, m_PhysicalDevice, createInfo,
                                           // TODO: Don't just assume first is good here
                                           m_GraphicsCommandBufferPool->CreateCommandBuffer(*m_GraphicsQueue)};
    }
    m_Swapchain->Recreate(m_SwapchainFramebuffers, newSize);
}

ShaderModule VulkanDevice::LoadShaderModule(const std::filesystem::path &filename)
{
    return ShaderModule::LoadFromDisk(m_Device, filename);
}

RasterPipeline VulkanDevice::CreateRasterPipeline(RasterPipelineBuilder &&pipelineBuilder, const RenderPass& renderPass)
{
    auto fragmentShader = LoadShaderModule(pipelineBuilder.GetFragmentShaderPath());
    auto vertexShader = LoadShaderModule(pipelineBuilder.GetVertexShaderPath());

    VkPipelineShaderStageCreateInfo fragCreateInfo{};
    fragCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragCreateInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
    fragCreateInfo.module = fragmentShader.Get();
    fragCreateInfo.pName = "main";
    fragCreateInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo vertexCreateInfo{};
    vertexCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexCreateInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
    vertexCreateInfo.module = vertexShader.Get();
    vertexCreateInfo.pName = "main";
    vertexCreateInfo.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo stages[] = {fragCreateInfo, vertexCreateInfo};

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_VIEWPORT,
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    auto bindingDescription = pipelineBuilder.GetVertexBindingDescription();
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = bindingDescription.has_value() ? bindingDescription->GetVkPipelineInputStateCreateInfo() 
        : VertexBindingDescription::DefaultPipelineInputStateCreateInfo();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // TODO: Get framebuffer from pipeline builder, so that it's not 
    // fixed to the currently active swapchain on this device.
    assert(m_Swapchain.has_value());

    Viewport viewport = m_Swapchain->GetViewportDescription();
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport.Viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &viewport.Scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    rasterizationState.lineWidth = 1.0f;
    rasterizationState.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp = 0.0f;
    rasterizationState.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multiSampleState{};
    multiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSampleState.sampleShadingEnable = VK_FALSE;
    multiSampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multiSampleState.minSampleShading = 1.0f;
    multiSampleState.pSampleMask = nullptr;
    multiSampleState.alphaToCoverageEnable = VK_FALSE;
    multiSampleState.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
    colorBlendAttachmentState.colorWriteMask =
        VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT |
        VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_TRUE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VkLogicOp::VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachmentState;
    for (uint32_t i = 0; i < 4; i++)
    {
        colorBlendState.blendConstants[i] = 0.0f;
    }

    VkPipelineDepthStencilStateCreateInfo depthStencilState{};
    depthStencilState.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VkCompareOp::VK_COMPARE_OP_LESS;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = {};
    depthStencilState.back = {};

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizationState;
    pipelineInfo.pMultisampleState = &multiSampleState;
    pipelineInfo.pDepthStencilState = &depthStencilState;

    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.pDynamicState = &dynamicState;
    auto descriptorSetLayout = pipelineBuilder.GetDescriptorSetLayout();
    
    std::vector<VkDescriptorSetLayout> layouts{};
    if (descriptorSetLayout.has_value())
    {
        layouts = {descriptorSetLayout->get().Get()};
    }
    
    auto createInfo = PipelineCreateInfo{pipelineInfo, layouts, renderPass};
    // TODO: Manage here so that you cannot destory a pipeline before destroying its
    // descriptor set (layout)
    return RasterPipeline(m_Device, createInfo);
}

VulkanDevice::VulkanDevice(PhysicalDevice &physicalDevice, VkPhysicalDevice physicalDeviceHandle,
                        const VulkanInstance& instance,
                        const std::vector<const char*> &validationLayers, std::vector<EDeviceExtension> extensions,
                        const DeviceExtensionMapping &deviceExtensionMapping, GLFWwindow& window)
    : m_Instance(instance), m_PhysicalDevice(physicalDevice), m_Window(window)
{
    assert(physicalDevice.IsValid() && "Need a valid physical device");

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos = GetQueueCreateInfos(physicalDevice);

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    // Also specify here for backwards compatability with old vulkan implementations.
    // This shouldn't functionally change the enabled validation layers
    if (!validationLayers.empty())
    {
        deviceCreateInfo.enabledLayerCount = (uint32_t)validationLayers.size();
        
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }
    auto extensionNames = deviceExtensionMapping.ReverseMap(std::span<EDeviceExtension>{extensions});
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

    deviceCreateInfo.ppEnabledExtensionNames = extensionNames.data();

    deviceCreateInfo.pEnabledFeatures = &physicalDevice.GetFeatures();

    if (vkCreateDevice(physicalDeviceHandle, &deviceCreateInfo, nullptr, &m_Device) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create logical device");
    }
    // Assertion: physical device has a graphics and present family queue
    m_GraphicsQueue = Queue(m_Device, physicalDevice.GetQueueFamilies().GraphicsFamilyIndex.value());
    m_PresentQueue = Queue(m_Device, physicalDevice.GetQueueFamilies().PresentFamilyIndex.value());

    // Possibly redundant creation if it's shared with the graphics queue
    m_TransferQueue = Queue(m_Device, physicalDevice.GetQueueFamilies().TransferFamilyIndex.value());
    
    m_GraphicsCommandBufferPool = std::make_unique<CommandBufferPool>(CreateGraphicsCommandBufferPool());
    m_TransferCommandBufferPool = std::make_unique<CommandBufferPool>(CreateTransferCommandBufferPool());
    m_DescriptorPool = std::make_unique<DescriptorPool>(
        // Arbitrary size
        m_Device, DescriptorPoolCreateInfo{64, {VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}});
}

VulkanDevice::VulkanDevice(VulkanDevice &&other)
    : m_Device(std::exchange(other.m_Device, VK_NULL_HANDLE)), m_PhysicalDevice(other.m_PhysicalDevice),
      m_GraphicsQueue(other.m_GraphicsQueue), m_PresentQueue(other.m_PresentQueue),
      m_TransferQueue(other.m_TransferQueue),
      m_Swapchain(std::move(other.m_Swapchain)), 
      m_GraphicsCommandBufferPool(std::move(other.m_GraphicsCommandBufferPool)),
      m_TransferCommandBufferPool(std::move(other.m_TransferCommandBufferPool)),
      m_Semaphores(std::move(other.m_Semaphores)),
      m_SwapchainFramebuffers(std::move(other.m_SwapchainFramebuffers)), m_Window(other.m_Window),
      m_DescriptorPool(std::move(other.m_DescriptorPool)),
      m_Instance(other.m_Instance)
{
}

VulkanDevice::~VulkanDevice()
{
    if (m_Device == VK_NULL_HANDLE)
    {
        // Moved
        return;
    }
    std::cout << "Destroying vk device";
    if (m_PresentQueue.has_value())
    {
        // The present queue may still be in the process of performing
        // present calls. There is no sema or fence to wait for these explicitly,
        // so just wait for all its operations to complete.
        // This is valid for other queues, but we should prefer explicitly
        // ordering through semaphores and fences for specific operations instead.
        m_PresentQueue->Wait();
    }

    m_DescriptorSetLayouts.clear();
   
    m_DescriptorPool.reset();
    // Explicitly order destruction of vulkan objects
    // Prior to swapchain destruction, since framebuffers may be 
    // to swapchain images
    m_SwapchainFramebuffers.clear();

    m_Swapchain.reset();
    m_GraphicsCommandBufferPool.reset();
    m_TransferCommandBufferPool.reset();
    m_Semaphores.clear();
    m_VertexBuffers.clear();
    m_IndexBuffers.clear();
    m_Buffers.clear();
    m_Textures.clear();

    std::condition_variable destroyed;
    std::mutex destroyMutex;
    std::thread destroyThread([this, &destroyed, &destroyMutex] {
        std::unique_lock lock(destroyMutex);
        vkDeviceWaitIdle(m_Device);
        destroyed.notify_one();
    });
    std::unique_lock lock(destroyMutex);
    if (destroyed.wait_for(lock, std::chrono::milliseconds(500)) == std::cv_status::timeout)
    {
        std::cout << "ERROR: Waited for > 500 ms for queue operations to finish. Forcibly deleting device";
        // Detach, not going to wait for a blocking call. We already announced we're forcefully
        // deleting the device here.
        destroyThread.detach();
    }
    else
    {
        destroyThread.join();
    }
    vkDestroyDevice(m_Device, nullptr);
}

RenderPass VulkanDevice::CreateRenderPass(DepthAttachment& depthAttachment)
{
    // TODO: Don't always bind this to the swapchain
    assert(m_Swapchain.has_value());
    auto attachmentDescription = m_Swapchain->AttachmentDescription();
    
    return RenderPass(m_Device,
                      RenderPassCreateInfo{attachmentDescription, depthAttachment.GetAttachmentDescription()});
}

const SwapchainFramebuffer& VulkanDevice::CreateSwapchainFramebuffers(const RenderPass &renderpass, DepthAttachment *depthAttachment)
{
    assert(m_Swapchain.has_value() && "No swapchain to create framebuffers for");
    return *m_SwapchainFramebuffers.emplace_back(std::make_unique<SwapchainFramebuffer>(m_Swapchain->CreateFramebuffersFor(renderpass, depthAttachment)));
}

CommandBufferPool VulkanDevice::CreateGraphicsCommandBufferPool()
{
    auto familyIndices = m_PhysicalDevice.GetQueueFamilies();
    assert(familyIndices.GraphicsFamilyIndex.has_value() &&
           "No graphics family queue to create command buffer pool for");

    CommandBufferPoolCreateInfo createInfo{VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                           familyIndices.GraphicsFamilyIndex.value()};
    auto commandBufferPool = CommandBufferPool{m_Device, createInfo, m_Instance};
    commandBufferPool.SetName("Graphics CMD Buffer Pool", m_Instance.GetExtensionFunctionMapping());
    return commandBufferPool;
}

CommandBufferPool VulkanDevice::CreateTransferCommandBufferPool() const
{
    auto familyIndices = m_PhysicalDevice.GetQueueFamilies();
    assert(familyIndices.TransferFamilyIndex.has_value() &&
           "No graphics family queue to create command buffer pool for");

    CommandBufferPoolCreateInfo createInfo{VkCommandPoolCreateFlagBits::VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
                                           familyIndices.TransferFamilyIndex.value()};

    auto commandBufferPool = CommandBufferPool(m_Device, createInfo, m_Instance);
    commandBufferPool.SetName("Transfer CMD Buffer Pool", m_Instance.GetExtensionFunctionMapping());
    return commandBufferPool;
}

CommandBuffer &VulkanDevice::GetTransferCommandBuffer()
{
    // TODO: Cache most recent?
    return m_TransferCommandBufferPool->CreateCommandBuffer(*m_TransferQueue);
}

CommandBufferPool &VulkanDevice::GetGraphicsCommandBufferPool()
{
    return *m_GraphicsCommandBufferPool;
}

Semaphore &VulkanDevice::CreateDeviceSemaphore()
{
    return *m_Semaphores.emplace_back(std::make_unique<Semaphore>(m_Device));
}

Queue VulkanDevice::GetGraphicsQueue() const
{
    assert(m_GraphicsQueue.has_value() && "Device has no graphics queue");
    return m_GraphicsQueue.value();
}

Queue VulkanDevice::GetTransferQueue() const
{
    assert(m_TransferQueue.has_value() && "Device has no transfer queue");
    return m_TransferQueue.value();
}

void VulkanDevice::AcquireNext(const Semaphore& toSignal)
{
    // TODO: Ensure semaphores in correct state, or more properly,
    // that we cannot have a recording cmd buffer at this time
    if (m_LastUnhandledResize)
    {   
        RecreateSwapchain(*m_LastUnhandledResize);
        m_LastUnhandledResize.reset();
    }
    assert(m_Swapchain.has_value());
    // Handle non-optimal swapchains in `Present` instead. Assume that reutrn SwapchainState::Suboptimal
    // as well when AcquireNext does
    while (m_Swapchain->AcquireNext(toSignal) == SwapchainState::OutOfDate)
    {
        RecreateSwapchain(SelectSwapchainExtent(m_Window, m_PhysicalDevice.QuerySurfaceProperties()));
    }
}

void VulkanDevice::Present(std::span<Semaphore> waitSemaphores)
{
    assert(m_Swapchain.has_value());
    if (m_Swapchain->Present(waitSemaphores) != SwapchainState::Optimal)
    {
        std::cout << "Present recreate\n";
        RecreateSwapchain(SelectSwapchainExtent(m_Window, m_PhysicalDevice.QuerySurfaceProperties()));
    }
}

void VulkanDevice::HandleResizeEvent(const WindowResizeEvent & resizeEvent)
{
    m_LastUnhandledResize = VkExtent2D{resizeEvent.NewWidth, resizeEvent.NewHeight};
}

ExtensionFunctionMapping VulkanDevice::GetExtensionFunctionMapping() const
{
    return m_Instance.GetExtensionFunctionMapping();
}

IndexBuffer &VulkanDevice::CreateIndexBuffer(std::vector<uint16_t> data)
{
    assert(m_GraphicsQueue.has_value() && "Need a graphics queue");
    CreateIndexBufferInfo info = CreateIndexBufferInfo(data, VkSharingMode::VK_SHARING_MODE_EXCLUSIVE, m_GraphicsQueue);

    auto &commandBuffer = GetTransferCommandBuffer();
    commandBuffer.SetName("Index Buffer Transfer Command Buffer", m_Instance.GetExtensionFunctionMapping());
    return *m_IndexBuffers.emplace_back(std::make_unique<IndexBuffer>(IndexBuffer(info, m_Device, m_PhysicalDevice, commandBuffer)));
}

std::vector<VkDeviceQueueCreateInfo> VulkanDevice::GetQueueCreateInfos(const PhysicalDevice &physicalDevice)
{
    std::set<uint32_t> uniqueQueueIndices = physicalDevice.GetQueueFamilies().GetUniqueQueues();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueQueueIndices.size());
    for (uint32_t queueIndex : uniqueQueueIndices)
    {
        VkDeviceQueueCreateInfo graphicsQueueCreateInfo{};
        graphicsQueueCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        graphicsQueueCreateInfo.queueFamilyIndex = queueIndex;
        graphicsQueueCreateInfo.queueCount = 1;
        // TODO: Fix this as it's a temporary. Possibly just translate 
        // queue create infos from own format
        static float priority = 1.0f;
        graphicsQueueCreateInfo.pQueuePriorities = &priority;
        queueCreateInfos.emplace_back(graphicsQueueCreateInfo);
    }
    return queueCreateInfos;
}

std::set<uint32_t> QueueFamilyIndices::GetUniqueQueues() const
{
    return {GraphicsFamilyIndex.value(), PresentFamilyIndex.value(), ComputeFamilyIndex.value(), TransferFamilyIndex.value()};
}
