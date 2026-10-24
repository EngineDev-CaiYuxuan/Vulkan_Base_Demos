#include "syncManager.h"
#include "utils.h"


SyncManager::SyncManager(const RenderContextDeps& rcd)
	:device(rcd.device)
{

}

void SyncManager::initialize()
{
	createSyncObjects();
}

SyncDeps SyncManager::getSyncDeps()
{
	SyncDeps sd(imageAvailableSemaphores, renderFinishedSemaphores, inFlightFences);
	return sd;
}

void SyncManager::cleanup()
{
	for (auto semaphore : imageAvailableSemaphores)
	{
		vkDestroySemaphore(device, semaphore, nullptr);
	}

	for (auto semaphore : renderFinishedSemaphores)
	{
		vkDestroySemaphore(device, semaphore, nullptr);

	}

	for (auto fence : inFlightFences)
	{
		vkDestroyFence(device, fence, nullptr);
	}
}

void SyncManager::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	//
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;//该版本仅有这一个必填字段

	//填写栅栏创建结构体
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;//初始状态设置为已触发

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		//创建信号量和栅栏
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS
			)
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}