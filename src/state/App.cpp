#include "App.hpp"

#include <iostream>
#include <map>

#include <torasu/torasu.hpp>

// XXX For creating an test-tree
#include <torasu/std/torasu_full.hpp>

#include "TreeManager.hpp"

namespace tstudio {

struct App::State {
	std::map<std::string, const torasu::ElementFactory*> elementFactories;
	TreeManager* treeManager;
	torasu::Element* root;
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
}

App::~App() {
	delete state->treeManager;
	delete state;
}

TreeManager* App::getTreeManager() {
	return state->treeManager;
}

torasu::Element* App::getRootElement() {
	return state->root;
}

} // namespace tstudio
