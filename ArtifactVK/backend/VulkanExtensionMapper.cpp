#include "VulkanExtensionMapper.h"

VulkanExtensionMapper::VulkanExtensionMapper(const VkInstance& vkInstance) :
	m_VkInstance(vkInstance),
	m_ExtensionFunctionMapping(CreateExtensionMapping(CreateExtensionNameMapping()))
{

}

std::unordered_map<EExtensionFunction, const char*> VulkanExtensionMapper::CreateExtensionNameMapping() const
{
	std::unordered_map<EExtensionFunction, const char*> nameMapping;
	nameMapping.insert({ EExtensionFunction::VkCreateDebugUtilsMessengerExt, "vkCreateDebugUtilsMessengerEXT" });
	nameMapping.insert({ EExtensionFunction::VkDestroyDebugUtilsMessengerEXT, "vkDestroyDebugUtilsMessengerEXT" });
	return nameMapping;
}

std::unordered_map<EExtensionFunction, PFN_vkVoidFunction> VulkanExtensionMapper::CreateExtensionMapping(const std::unordered_map<EExtensionFunction, const char*>& extensionNameMapping) const
{
	std::unordered_map<EExtensionFunction, PFN_vkVoidFunction> functionMapping;
	functionMapping.reserve(extensionNameMapping.size());
	for (const auto& [function, name] : extensionNameMapping) 
	{
		auto extension = vkGetInstanceProcAddr(m_VkInstance, name);
		if (extension != nullptr)
		{
			functionMapping[function] = extension;
		}
	}
	return functionMapping;
}


PFN_vkVoidFunction VulkanExtensionMapper::GetFunction(EExtensionFunction function) const
{
	// TODO: Graceful hndling of extensions that aren't available
	return m_ExtensionFunctionMapping.at(function);
}

