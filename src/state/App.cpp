#include "App.hpp"

#include <iostream>
#include <map>
#include <memory>

#include <torasu/torasu.hpp>

// XXX For creating an test-tree
#include <torasu/std/torasu_full.hpp>
#include <torasu/mod/imgc/imgc_full.hpp>

#include "TreeManager.hpp"
#include "RenderQueue.hpp"
#include "monitors/Monitor.hpp"
#include "monitors/NumberMonitor.hpp"
#include "monitors/ImageMonitor.hpp"

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
 
	auto* num1 = new torasu::tstd::Rnum(0.5);
	// auto* num2 = new torasu::tstd::Rnum(2.0);
	// auto* mul1 = new torasu::tstd::Rmultiply(num1, num2);
	// auto* mul2 = new torasu::tstd::Rmultiply(mul1, 10);
	// auto* sub1 = new torasu::tstd::Rsubtract(mul2, 20);
	auto* color1 = new imgc::Rcolor(1.0, 1.0, 0.8, num1);
	// auto* color2 = new imgc::Rcolor(1.0, num2, 0.3, 1.0);
	auto* video = new imgc::Rmedia_file(torasu::tools::inlineRenderable(new torasu::tstd::Rnet_file(
		"https://wasm.hcink.org/torasu/sample-mp4-bunny.mp4"
	)));
	auto* image = new torasu::tstd::Rmod_rctx(video, torasu::tools::inlineRenderable(new torasu::tstd::Rnum(0)), TORASU_STD_CTX_TIME, TORASU_STD_PL_NUM);
	auto* colorMul = new torasu::tstd::Rmultiply(image, color1);
	auto* text = new imgc::Rtext("TEST");
	auto* textRnd = new imgc::Rgraphics(text);
	auto* roundVal = new torasu::tstd::Rnum(1.0);
	auto* circle = new imgc::Rrothumbus(roundVal);
	auto* circleRnd = new imgc::Rgraphics(circle);
	auto* circleAlign = new imgc::Rauto_align2d(circleRnd, 0.0, 0.0, 0.0, 1.0);
	auto* circleTransform = new imgc::Rtransform(
		torasu::tools::inlineRenderable(circleAlign), 
		torasu::tools::inlineRenderable(new torasu::tstd::Rmatrix({1.0, 0.0, 0.0, 0.0, 1.0, 0.0}, 2)));
	auto* layers = new imgc::Rlayer(torasu::tools::inlineRenderable(
		new torasu::tstd::Rlist({
			circleTransform,
			colorMul,
			// textRnd,
		})
	));
	auto* encode = new imgc::Rmedia_creator(layers, "mp4", 0.0, 10.0, 25.0, 1280, 720, 4000*1000);

	state->treeManager = new TreeManager(state->elementFactories, 
		{/* imageFile, */ video, image, num1, /* num2, */ /* mul1, mul2, sub1, */ color1/* , color2 */, colorMul, text, textRnd, layers, encode, circleRnd, circle, roundVal, /* circleAlign, */ circleTransform},
		encode);
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
