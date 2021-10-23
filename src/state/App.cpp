#include "App.hpp"

#include <iostream>
#include <map>
#include <memory>

#include <torasu/torasu.hpp>

// XXX For creating an test-tree
#include <torasu/std/torasu_full.hpp>

#include "TreeManager.hpp"
#include "RenderQueue.hpp"

namespace tstudio {

struct App::State {
	std::map<std::string, const torasu::ElementFactory*> elementFactories;
	TreeManager* treeManager;
	RenderQueue* renderQueue;
	std::unique_ptr<torasu::tstd::EIcore_runner> runner;
	std::unique_ptr<torasu::ExecutionInterface> runnerInterface;
	std::unique_ptr<torasu::tstd::LIcore_logger> logger;
	torasu::Renderable* root;

	bool updateNumber = true;
	bool numberEunqueued = false;
	uint64_t renderId;
	torasu::RenderContext rctx;
	torasu::ResultSettings numSettings = torasu::ResultSettings(TORASU_STD_PL_NUM, torasu::tools::NO_FORMAT);
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
}

App::~App() {
	delete state->renderQueue;
	delete state->treeManager;
	delete state;
}

void App::onBlank(const tstudio::blank_callbacks& callbacks) {
	state->renderQueue->updatePendings();
	if (state->numberEunqueued) {
		RenderQueue::ResultState status = state->renderQueue->getResultState(state->renderId);
		if (status != RenderQueue::ResultState_PENDING) {
			torasu::RenderResult* result = state->renderQueue->fetchResult(state->renderId);
			state->numberEunqueued = false;
			if (result != nullptr) {
				auto* numVal = dynamic_cast<torasu::tstd::Dnum*>(result->getResult());
				if (numVal == nullptr) {
					static_cast<torasu::LogInterface*>(state->logger.get())->log(torasu::LogLevel::ERROR, "Render did not return expected type!");
				}
				currentNumber = numVal->getNum();
				delete result;
			} else if (status == RenderQueue::ResultState_CANCELED) {
				state->updateNumber = true;
			} else {
				static_cast<torasu::LogInterface*>(state->logger.get())->log(torasu::LogLevel::ERROR, "Result has not been returned!");
			}
		}
	}
	if (state->treeManager->hasUpdates() && state->renderQueue->requestPause()) {
		state->treeManager->applyUpdates();
		state->renderQueue->resume();
		state->updateNumber = true;
	}
	if (state->updateNumber && state->renderQueue->mayEnqueue()) {
		state->updateNumber = false;
		torasu::LogInstruction li(state->logger.get(), torasu::INFO);
		state->renderId = state->renderQueue->enqueueRender(state->root, &state->rctx, &state->numSettings, li);
		state->updateNumber = false;
		state->numberEunqueued = true;
	}
}

TreeManager* App::getTreeManager() {
	return state->treeManager;
}

torasu::Renderable* App::getRootElement() {
	return state->root;
}

} // namespace tstudio
