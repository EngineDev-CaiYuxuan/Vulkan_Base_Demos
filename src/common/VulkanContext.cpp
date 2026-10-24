#include "vulkanContext.h"
#include "utils.h"

void VulkanContext::initialize()
{
	initWindow();
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicDevice();
}

void VulkanContext::cleanup()
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(device, nullptr);
	DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
}

RenderContextDeps VulkanContext::getContextDeps()
{
	RenderContextDeps deps(device, physicalDevice, surface, window, msaaSamples, graphicsQueue, presentQueue, computeQueue);
	return deps;
}

VkDevice& VulkanContext::getDevice() { return device; }
GLFWwindow* VulkanContext::getWindow() { return window; }
bool& VulkanContext::getFrameBool() { return framebufferResized; }

void VulkanContext::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<VulkanContext*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}
void VulkanContext::initWindow()
{
	//初始化GLFW库
	glfwInit();

	//不要使用后续调用来创建 OpenGL 上下文
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // 改为允许缩放

	//创建实际窗口
	window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan", nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);
	//设置回调函数
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VulkanContext::recreateWindow()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);//持续获取窗口大小参数
		glfwWaitEvents();
	}

	//等待GPU完成上一帧
	vkDeviceWaitIdle(device);
}

void VulkanContext::createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; //结构体类型 必填
	appInfo.pApplicationName = "Hello Triangle"; //应用名称
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); //应用版本号
	appInfo.pEngineName = "No Engine"; //引擎名称
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0); //引擎版本号
	appInfo.apiVersion = VK_API_VERSION_1_0; //将要使用的VulkanAPI版本 必填

	/*获取拓展列表*/
	auto extensions = getRequiredExtensions();
	if (!checkAllRequiredExtensionsSupported(extensions))
	{
		throw std::runtime_error("Some required Vulkan extensions are not support!");
	}
	//检查验证层支持
	if (enableValidationLayers && !checkValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}

	/*将应用信息、拓展列表、验证层列表打包*/
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;//结构体类型 必填
	createInfo.pApplicationInfo = &appInfo;//指向应用信息结构体 必填
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());//要启用的拓展数量 必填
	createInfo.ppEnabledExtensionNames = extensions.data();//拓展列表指针 必填
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;//?
	}
	else
	{
		createInfo.enabledLayerCount = 0;//要启用的验证层的数量

		createInfo.pNext = nullptr;
	}

	/*发出vkCreateInstance函数调用*/
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	/*Vulkan不会自动抛出异常 每一个VulkanAPI调用都必须检查返回值 尤其是对象创建的*/
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create instance");
	}
}

/*检查拓展支持情况*/
bool VulkanContext::checkAllRequiredExtensionsSupported(
	const std::vector<const char*>& requiredExtensions)
{
	uint32_t supportedCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &supportedCount, nullptr);

	std::vector<VkExtensionProperties> supportedExtensions(supportedCount);
	vkEnumerateInstanceExtensionProperties(
		nullptr, &supportedCount, supportedExtensions.data());

	for (const char* required : requiredExtensions)
	{
		bool found = false;
		for (const auto& supported : supportedExtensions)
		{
			if (!strcmp(required, supported.extensionName))
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			std::cerr << "ERROR: Required extension not supported: " << required << std::endl;
			return false;
		}
	}

	std::cout << "All required extensions are suppored!" << std::endl;
	return true;
}

/*检查验证层支持*/
bool VulkanContext::checkValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

/*根据是否开启验证层确定返回的拓展列表*/
std::vector<const char*> VulkanContext::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

/*回调函数（静态）*/
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,//指定消息的严重程度
	VkDebugUtilsMessageTypeFlagsEXT messageType,//消息类型
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,//消息详情 一个结构体指针 包含具体的错误信息
	void* pUserData)//万能指针 传用户自己的数据 在回调函数中使用
{
	std::cerr << "validation layer" << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void VulkanContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
}

void VulkanContext::setupDebugMessenger()
{
	if (!enableValidationLayers) return;//未启用则直接返回

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set dubug messenger!");
	}
}

void VulkanContext::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);//查询数量
	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice>devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	for (const auto device : devices)
	{
		if (isDeviceSuitable(device))
		{
			physicalDevice = device;
			msaaSamples = getMaxUsableSampleCount(physicalDevice);//设置最大多重采样值
			break;
		}
	}
	if (physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device, surface);

	bool extensionsSupported = checkDeviceExtensionSupported(device);

	//检查拓展支持情况
	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainDetails details = querySwapChainSupported(device, surface);
		swapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
	}

	//采样器支持检测
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.graphicsAndComputeFamily.has_value() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool VulkanContext::checkDeviceExtensionSupported(VkPhysicalDevice device)//检查拓展支持
{
	//查询拓展数量
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	//创建拓展容器并查询内容
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	//获取所需拓展
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	//遍历筛选
	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName); //同名反向排除
	}

	return requiredExtensions.empty(); //如果为空则说明全部支持
}

/*创建逻辑设备*/
void VulkanContext::createLogicDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsAndComputeFamily.value(), //注意区分has_value() 与 value()
		indices.presentFamily.value()
	};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};//指定使用的设备功能
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE; // enable sample shading feature for the device

	//填写主要结构体
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();//指向队列创建信息的指针
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());//创建的队列数量
	createInfo.pEnabledFeatures = &deviceFeatures;//指向设备功能的指针
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());//指定特定于设备的拓展数量
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	//获取队列的句柄
	vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &computeQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void VulkanContext::createSurface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}