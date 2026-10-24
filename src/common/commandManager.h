#pragma once

#include <iostream>

class RenderContextDeps;
class CommandDeps;

class CommandManager
{
public:
	CommandManager(const RenderContextDeps& rcd);
	void initialize();
	void cleanup();
	CommandDeps getCommandDeps();

	void createCommandPool();
	void createCommandBuffers();

	
private:
	const VkPhysicalDevice& physicalDevice;
	const VkDevice& device;
	const VkSurfaceKHR& surface;


	VkCommandPool commandPool;

	std::vector<VkCommandBuffer> commandBuffers;
};

