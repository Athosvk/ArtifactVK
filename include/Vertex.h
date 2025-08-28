#pragma once
#include <array>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <backend/Pipeline.h>

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Color;
    glm::vec2 UV;

    constexpr static VkVertexInputBindingDescription GetBindingDescription();
    constexpr static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions(); 
    constexpr static VertexBindingDescription GetVertexBindingDescription();
};

constexpr VkVertexInputBindingDescription Vertex::GetBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription;
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

constexpr std::array<VkVertexInputAttributeDescription, 3> Vertex::GetAttributeDescriptions()
{
    VkVertexInputAttributeDescription positionAttribute;
    positionAttribute.binding = 0;
    positionAttribute.location = 0;
    positionAttribute.format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
    positionAttribute.offset = offsetof(Vertex, Position);

    VkVertexInputAttributeDescription colorAttribute;
    colorAttribute.binding = 0;
    colorAttribute.location = 1;
    colorAttribute.format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
    colorAttribute.offset = offsetof(Vertex, Color);

    VkVertexInputAttributeDescription uvAttribute;
    uvAttribute.binding = 0;
    uvAttribute.location = 2;
    uvAttribute.format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
    uvAttribute.offset = offsetof(Vertex, UV);
    return {positionAttribute, colorAttribute, uvAttribute};
}

constexpr VertexBindingDescription Vertex::GetVertexBindingDescription()
{
    return VertexBindingDescription{
        GetBindingDescription(),
        GetAttributeDescriptions()
    };
}
