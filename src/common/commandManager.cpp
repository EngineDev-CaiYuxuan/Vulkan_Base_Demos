#include "utils.h"
#include "commandManager.h"


CommandManager::CommandManager(const RenderContextDeps& rcd)
	:physicalDevice(rcd.physicalDevice), device(rcd.device), surface(rcd.surface)
{

}

void CommandManager::initialize()
{
	createCommandPool();
	createCommandBuffers();
}

void CommandManager::cleanup()
{
	vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
	vkDestroyCommandPool(device, commandPool, nullptr);
}

CommandDeps CommandManager::getCommandDeps()
{
	CommandDeps cd(commandPool, commandBuffers);
	return cd;
}

//创建命令池
void CommandManager::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	//创建命令池结构体
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//命令池标志 共两种 此标志表示允许单独重新记录命令缓冲区
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();//记录绘图命令的命令池分配到绘图队列上

	//提交创建
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}
//从命令池中分配单个命令缓冲区
void CommandManager::createCommandBuffers()
{
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	//
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;//绑定命令池子
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;//指定分配的命令缓冲区是主缓冲区还是辅助缓冲区
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();//分配命令缓冲区的数量

	//分配命令缓冲区
	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}