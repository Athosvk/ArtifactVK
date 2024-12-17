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
	explicit VulkanExtensionMapper(const VkInstance& vkInstance);
private:
	std::unordered_map<EExtensionFunction, const char*> CreateFunctionNameMapping() const;
	std::unordered_map<EExtensionFunction, PFN_vkVoidFunction> CreateFunctionMapping(const std::unordered_map<EExtensionFunction, const char*>& extensionNameMapping) const;
	PFN_vkVoidFunction GetFunction(EExtensionFunction function) const;

	const VkInstance& m_VkInstance;
	std::unordered_map<EExtensionFunction, PFN_vkVoidFunction> m_ExtensionFunctionMapping;
};

