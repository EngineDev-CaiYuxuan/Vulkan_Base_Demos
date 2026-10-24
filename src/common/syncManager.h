#pragma once

#include "utils.h"

class RenderContextDeps;
class SyncDeps;

class SyncManager
{
public:
	SyncManager(const RenderContextDeps& rcd);
	void initialize();
	void cleanup();
	SyncDeps getSyncDeps();

	void createSyncObjects();

private:
	const VkDevice& device;


	std::vector<VkSemaphore> imageAvailableSemaphores;

	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkFence> inFlightFences;
};