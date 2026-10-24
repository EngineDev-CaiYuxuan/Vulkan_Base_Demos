#include "swapchainManager.h"
#include "utils.h"

#include <algorithm>



SwapchainManager::SwapchainManager(RenderContextDeps& deps) :
	physicalDevice(deps.physicalDevice), device(deps.device), surface(deps.surface), window(deps.window)
{

}

SwapchainDeps SwapchainManager::getSwapchainDeps()
{
	SwapchainDeps sd(swapChain, swapChainImageFormat, swapChainExtent, swapChainImageViews, swapChainImages);
	return sd;
}

void SwapchainManager::initialize()
{
	createSwapChain();
	createImageViews();
}

void SwapchainManager::cleanup()
{
	for (VkImageView view : swapChainImageViews)
	{
		if (view != VK_NULL_HANDLE)
		{
			vkDestroyImageView(device, view, nullptr);
		}
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void SwapchainManager::cleanForRecreate()
{
	//清理图像视图
	for (auto imageView : swapChainImageViews)
	{
		vkDestroyImageView(device, imageView, nullptr);
	}

	//清除交换链
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

//查找最优表面格式
VkSurfaceFormatKHR SwapchainManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	//遍历并判断最符合要求的表面格式
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
			&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	//如果没有理想格式 通常直接选择第一个
	return availableFormats[0];
}
//查找最优呈现模式
VkPresentModeKHR SwapchainManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)//三重缓冲
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;//垂直同步
}
//查找最优选择范围
VkExtent2D SwapchainManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;//不需要自定义时直接返回
	}
	else//窗口管理器允许任意分辨率时
	{
		//获取窗口的实际像素分辨率（也就是帧缓冲区的大小）
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		//填充结构体
		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		//限制分辨率在显卡允许范围内
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}
//创建交换链
void SwapchainManager::createSwapChain()
{
	//获取交换链细节
	SwapChainDetails swapChainSupport = querySwapChainSupported(physicalDevice, surface);

	//查找最优交换链参数
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	//设置交换链图像数量 建议至少为最小可运行数量+1
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	//验证图像数量不超过最大值（此处0为特殊值 表示没有最大值）
	if (swapChainSupport.capabilities.maxImageCount > 0
		&& imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	//填写大型结构
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;//对图像应用某种变换
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;//是否使用alpha通道
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;//是否裁剪被其他窗口遮挡的像素的颜色 通常选择true
	createInfo.oldSwapchain = VK_NULL_HANDLE;//旧交换链的引用 用于创建新交换链时传递字段

	//指定如何处理多个队列族之间使用的交换链图像
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);//获取队列族
	uint32_t queueFamilyIndices[] = {
		indices.graphicsAndComputeFamily.value(),
		indices.presentFamily.value()
	};
	if (indices.graphicsAndComputeFamily != indices.presentFamily)//当队列族不是同一个时
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;//一个图像一次只由一个队列族拥有 需要显式移交所有权 性能最佳
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else//队列族为同一个时
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	//检索交换链图像
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

//为交换链中的每个图像创建图像视图
void SwapchainManager::createImageViews()
{
	//调整列表大小
	swapChainImageViews.resize(swapChainImages.size());

	for (uint32_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = createImageView(device, swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}


