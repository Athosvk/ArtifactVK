#include "ShaderModule.h"

#include <fstream>
#include <iostream>

ShaderModule::ShaderModule(const VkDevice& vkDevice, const std::vector<char>& bytes) : m_Device(vkDevice)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytes.size();

    // TODO: Require buffer to be of right type for proper alignment
    createInfo.pCode = reinterpret_cast<const uint32_t*>(bytes.data());
    if (vkCreateShaderModule(vkDevice, &createInfo, nullptr, &m_ShaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Could not create shader module");
    }
}

ShaderModule::~ShaderModule()
{
    vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
}

ShaderModule ShaderModule::LoadFromDisk(const VkDevice& vkDevice, const std::filesystem::path &filename)
{
    std::cout << "Cwd: " << std::filesystem::current_path();
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error(std::string("Couldn't open file at ") + filename.string());
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> fileContents(fileSize);
    file.seekg(0);
    file.read(fileContents.data(), fileSize);
    file.close();
    std::cout << "Read file contents of " << filename << " with size " << fileSize;

    return ShaderModule(vkDevice, fileContents);
}
