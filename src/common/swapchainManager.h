#pragma once

#include "VulkanContext.h"

#include <iostream>

class RnederContextDeps;
class SwapchainDeps;

class SwapchainManager
{
public:
	SwapchainManager(RenderContextDeps& deps);
	SwapchainDeps getSwapchainDeps();
	void initialize();
	void cleanup();
	void cleanForRecreate();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void createSwapChain();

	void createImageViews();


private:
	const VkPhysicalDevice& physicalDevice;
	const VkDevice& device;
	const VkSurfaceKHR&  surface;
	GLFWwindow* const window;

	VkSwapchainKHR swapChain;

	VkFormat swapChainImageFormat;

	VkExtent2D swapChainExtent;

	std::vector<VkImageView> swapChainImageViews;

	std::vector<VkImage> swapChainImages;
};