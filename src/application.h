#pragma once

#include "common/utils.h"
#include "common/VulkanContext.h"
#include "common/swapchainManager.h"
#include "common/RenderPassManager.h"
#include "common/commandManager.h"
#include "common/syncManager.h"
#include "demos/model_raster/modelRender.h"
#include "demos/particle_system/particleSystem.h"

class ApplicationBase
{
public:
	virtual ~ApplicationBase() = default;
	virtual void initialize() = 0;
	virtual void run() = 0;
	virtual void cleanup() = 0;
};


class ModelApplication : public ApplicationBase
{
public:
	void run();

private:
	void initialize();
	void mainLoop();
	void cleanup();
	void cleanupSwapChain();
	void recreateSwapChain();

private:
	VulkanContext* vc;
	SwapchainManager* scm;
	RenderPassManager* rpm;
	CommandManager* cm;
	SyncManager* sym;
	ModelRender* render;
};

class ParticleApplication : public ApplicationBase
{
public:
	void run();

private:
	void initialize();
	void mainLoop();
	void cleanup();
	void cleanupSwapChain();
	void recreateSwapChain();

private:
	VulkanContext* vc;
	SwapchainManager* scm;
	RenderPassManager* rpm;
	CommandManager* cm;
	SyncManager* sym;
	ParticleSystem* render;
};