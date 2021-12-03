#include "App.hpp"

#include <iostream>
#include <map>
#include <memory>

#include <torasu/torasu.hpp>
#include <torasu/std/EIcore_runner.hpp>
#include <torasu/std/LIcore_logger.hpp>

#include "TreeManager.hpp"
#include "RenderQueue.hpp"
#include "monitors/Monitor.hpp"
#include "monitors/NumberMonitor.hpp"
#include "monitors/ImageMonitor.hpp"

#include "examples/Examples.cpp"

namespace tstudio {

struct App::State {
	std::map<std::string, const torasu::ElementFactory*> elementFactories;
	TreeManager* treeManager;
	RenderQueue* renderQueue;
	std::unique_ptr<torasu::tstd::EIcore_runner> runner;
	std::unique_ptr<torasu::ExecutionInterface> runnerInterface;
	std::unique_ptr<torasu::tstd::LIcore_logger> logger;
	std::unique_ptr<Monitor> mainMonitor;
	std::vector<Monitor*> monitors;
	bool clearingCache = false;
};

extern "C" {
	const torasu::DiscoveryInterface* TORASU_DISCOVERY_torasu_core();
	const torasu::DiscoveryInterface* TORASU_DISCOVERY_torasu_std();
	const torasu::DiscoveryInterface* TORASU_DISCOVERY_imgc();
}

static const torasu::DiscoveryFunction DISCOVERY_FUNCTIONS[] = {
	TORASU_DISCOVERY_torasu_core,
	TORASU_DISCOVERY_torasu_std,
	TORASU_DISCOVERY_imgc,
};

App::App() {
	std::cout << "Init app..." << std::endl;
	state = new App::State();

	state->logger = std::unique_ptr<torasu::tstd::LIcore_logger>(new torasu::tstd::LIcore_logger(torasu::tstd::LIcore_logger::BASIC_CLOLORED));

	for (auto discoveryFunction : DISCOVERY_FUNCTIONS) {
		const torasu::DiscoveryInterface* torasuModule = discoveryFunction();
		auto moduleFactories = *torasuModule->getFactoryIndex();
		for (size_t i = 0; i < moduleFactories.elementFactoryCount; i++) {
			auto* factory = moduleFactories.elementFactoryIndex[i];
			state->elementFactories[factory->getType().str] = factory; 
		}
	}
	std::cout << "Loaded " << std::to_string(state->elementFactories.size()) 
				<< " element-types" << std::endl;
	
	examples::LoadedExample loadedExample = examples::makeSelectedExample();

	state->treeManager = new TreeManager(state->elementFactories, loadedExample.managedElements, loadedExample.root);
	state->runner = std::unique_ptr<torasu::tstd::EIcore_runner>(new torasu::tstd::EIcore_runner((size_t)1));
	state->runnerInterface = std::unique_ptr<torasu::ExecutionInterface>(state->runner->createInterface());
	state->renderQueue = new RenderQueue(state->runnerInterface.get());

	// state->mainMonitor = std::unique_ptr<Monitor>(new Monitor(new NumberMonitor(), state->logger.get()));
	state->mainMonitor = std::unique_ptr<Monitor>(new Monitor(new ImageMonitor(), state->logger.get()));
	state->mainMonitor->setRenderable(state->treeManager->getOutputNode()->getEffective());
	state->monitors.push_back(state->mainMonitor.get());
}

App::~App() {
	delete state->renderQueue;
	delete state->treeManager;
	delete state;
}

void App::onBlank(const tstudio::blank_callbacks& callbacks) {
	state->renderQueue->updatePendings();

	for (auto monitor : state->monitors) {
		monitor->fetchItems(state->renderQueue);
	}

	bool hasUpdates = state->treeManager->hasUpdates();
	if ((hasUpdates || state->clearingCache) && state->renderQueue->requestPause()) {
		if (hasUpdates) {
			state->treeManager->applyUpdates();
			for (auto monitor : state->monitors) {
				if (monitor->selectionMode == Monitor::MONITOR_ROOT) {
					monitor->setRenderable(state->treeManager->getOutputNode()->getEffective());
				}
			}
		}
		if (state->clearingCache) {
			state->runner->clearCache();
			state->clearingCache = false;
		}
		state->renderQueue->resume();

		for (auto monitor : state->monitors) {
			monitor->notifiyTreeUpdate();
		}
	}

	for (auto monitor : state->monitors) {
		monitor->enqueueItems(state->renderQueue);
	}
}

App::RunnerMetrics App::getRunnerMetrics() {
	auto metrics = state->runner->getMetrics();
	return {
		.queueSize = metrics.queueSize,
		.cacheItemCount = metrics.cacheItemCount,
		.cacheMemoryUsed = metrics.cacheMemoryUsed,
		.cacheMemoryMax = metrics.cacheMemoryMax,
		.clearingCache = state->clearingCache,
	};
}

void App::clearRunnerCache() {
	state->clearingCache = true;
}

TreeManager* App::getTreeManager() {
	return state->treeManager;
}

Monitor* App::getMainMonitor() {
	return state->mainMonitor.get();
}

} // namespace tstudio
