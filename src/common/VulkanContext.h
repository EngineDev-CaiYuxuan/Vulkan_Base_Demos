#pragma once

#define GLFW_INCLUDE_VULKAN

#include <iostream>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <optional>
#include <set>

class RenderContextDeps;

class VulkanContext
{
public:
	void initialize();
	void cleanup();
	RenderContextDeps getContextDeps();
	void initWindow();
	void recreateWindow();
	VkDevice& getDevice();
	GLFWwindow* getWindow();
	bool& getFrameBool();


	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	void createInstance();

	bool checkAllRequiredExtensionsSupported(const std::vector<const char*>& requiredExtensions);

	bool checkValidationLayerSupport();

	std::vector<const char*> getRequiredExtensions();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,//指定消息的严重程度
		VkDebugUtilsMessageTypeFlagsEXT messageType,//消息类型
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,//消息详情 一个结构体指针 包含具体的错误信息
		void* pUserData//万能指针 传用户自己的数据 在回调函数中使用
		);

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	void setupDebugMessenger();

	void pickPhysicalDevice();

	bool isDeviceSuitable(VkPhysicalDevice device);

	bool checkDeviceExtensionSupported(VkPhysicalDevice device);

	void createLogicDevice();
	
	void createSurface();

private:
	/*使用宏来控制是否启用验证层*/
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkInstance instance;

	VkDebugUtilsMessengerEXT debugMessenger;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue computeQueue;
	VkQueue presentQueue;

	VkSurfaceKHR surface;

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;
	GLFWwindow* window;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	bool framebufferResized = false;
};