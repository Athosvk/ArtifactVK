#pragma once
#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "backend/VulkanInstance.h"
#include "backend/Window.h"

class App
{
public:
	App();
	~App();

	void RunRenderLoop();
private:
	VulkanInstance m_VulkanInstance;
	Window m_Window;
};

