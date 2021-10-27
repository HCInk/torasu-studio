#include "App.hpp"

#include <iostream>
#include <map>
#include <memory>

#include <torasu/torasu.hpp>

// XXX For creating an test-tree
#include <torasu/std/torasu_full.hpp>

#include "TreeManager.hpp"
#include "RenderQueue.hpp"
#include "monitors/Monitor.hpp"
#include "monitors/NumberMonitor.hpp"

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
	torasu::Renderable* root;
};

extern "C" {
	const torasu::DiscoveryInterface* TORASU_DISCOVERY_torasu_core();
	const torasu::DiscoveryInterface* TORASU_DISCOVERY_torasu_std();
}

static const torasu::DiscoveryFunction DISCOVERY_FUNCTIONS[] = {
	TORASU_DISCOVERY_torasu_core,
	TORASU_DISCOVERY_torasu_std
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

	auto* num1 = new torasu::tstd::Rnum(2.0);
	auto* num2 = new torasu::tstd::Rnum(2.0);
	auto* mul1 = new torasu::tstd::Rmultiply(num1, num2);
	auto* mul2 = new torasu::tstd::Rmultiply(mul1, 10);

	state->root = mul2;
	state->treeManager = new TreeManager(state->elementFactories, 
		{num1, num2, mul1, mul2});

	state->runner = std::unique_ptr<torasu::tstd::EIcore_runner>(new torasu::tstd::EIcore_runner((size_t)1));
	state->runnerInterface = std::unique_ptr<torasu::ExecutionInterface>(state->runner->createInterface());
	state->renderQueue = new RenderQueue(state->runnerInterface.get());

	state->mainMonitor = std::unique_ptr<Monitor>(new Monitor(new NumberMonitor(), state->logger.get()));
	state->mainMonitor->setRenderable(state->root);
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

	if (state->treeManager->hasUpdates() && state->renderQueue->requestPause()) {
		state->treeManager->applyUpdates();
		state->renderQueue->resume();

		for (auto monitor : state->monitors) {
			monitor->notifiyTreeUpdate();
		}
	}

	for (auto monitor : state->monitors) {
		monitor->enqueueItems(state->renderQueue);
	}
}

TreeManager* App::getTreeManager() {
	return state->treeManager;
}

torasu::Renderable* App::getRootElement() {
	return state->root;
}

Monitor* App::getMainMonitor() {
	return state->mainMonitor.get();
}

} // namespace tstudio
