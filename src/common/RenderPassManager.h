#pragma once

#include "VulkanContext.h"
#include "swapchainManager.h"
#include "utils.h"

#include <iostream>


class RenderPassManager
{
public:
	RenderPassManager(const RenderContextDeps& rcd, const SwapchainDeps& sd);
	void initialize();
	RenderPassDeps getRenderPassDeps();
	void cleanup();
	void cleanForRecreate();

	void createColorResources();
	void createDepthResources();
	void createRenderPass();
	void createFramebuffers();

private:
	const VkPhysicalDevice& physicalDevice;
	const VkDevice& device;
	const VkSampleCountFlagBits& msaaSamples;

	const VkSwapchainKHR& swapChain;
	const VkFormat&	swapChainImageFormat;
	const VkExtent2D& swapChainExtent;
	const std::vector<VkImageView>& swapChainImageViews;
	const std::vector<VkImage>&	swapChainImages;

	//深度缓冲资源
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;

	VkRenderPass renderPass;

	std::vector<VkFramebuffer> swapChainFramebuffers;
};