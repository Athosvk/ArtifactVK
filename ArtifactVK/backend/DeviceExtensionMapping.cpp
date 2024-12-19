#include "DeviceExtensionMapping.h"

#include <vulkan/vulkan.h>

DeviceExtensionMapping::DeviceExtensionMapping() :
	m_NameMapping(CreateNameMapping())
{
}

EDeviceExtension DeviceExtensionMapping::At(std::string_view extensionName) const
{
	auto findIter = m_NameMapping.find(extensionName);
	return findIter != m_NameMapping.end() ? findIter->second : EDeviceExtension::Unknown;
}	

std::unordered_map<std::string_view, EDeviceExtension> DeviceExtensionMapping::CreateNameMapping()
{
	return {
		{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, EDeviceExtension::VkSwapchain }
	};
}
