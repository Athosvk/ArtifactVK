#pragma once
#include <unordered_map>
#include <vulkan/vulkan.h>

enum class EExtensionFunction
{
	VkCreateDebugUtilsMessengerExt,
	VkDestroyDebugUtilsMessengerEXT
};

class VulkanExtensionMapper
{
public:
	VulkanExtensionMapper(const VkInstance& vkInstance);
	std::unordered_map<EExtensionFunction, const char*> CreateExtensionNameMapping() const;
	std::unordered_map<EExtensionFunction, PFN_vkVoidFunction> CreateExtensionMapping(const std::unordered_map<EExtensionFunction, const char*>& extensionNameMapping) const;
	PFN_vkVoidFunction GetFunction(EExtensionFunction function) const;

private:
	const VkInstance& m_VkInstance;
	std::unordered_map<EExtensionFunction, PFN_vkVoidFunction> m_ExtensionFunctionMapping;
};

