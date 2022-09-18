#include "ActionBatch.hpp"

#include <stdexcept>
#include <algorithm>

namespace tstudio {

ActionBatch::ActionBatch(std::vector<UserAction*> actions, bool revertable) 
	: actions(actions), revertable(revertable) {}

ActionBatch::~ActionBatch() {
	for (auto action : actions) {
		delete action;
	}
}

UserAction::DependncyUpdateResult ActionBatch::notifyDependencyRemoval(App* instance, void* removed) {
	for (auto action : actions) {
		DependncyUpdateResult updateResult = action->notifyDependencyRemoval(instance, removed);
		if (updateResult == UNAVAILABLE) return UNAVAILABLE;
	}
	return AVAILABLE;
}

UserAction* ActionBatch::execute(App* instance, bool generateReverse) {
	if (generateReverse && revertable) { // Run action and generate reverse-action
		std::vector<UserAction*> reverseActions;
		bool revertPossible = true;
		for (auto action : actions) {
			auto reverseAction = action->execute(instance, true);
			if (reverseAction != nullptr) {
				reverseActions.push_back(reverseAction);
			} else {
				revertPossible = false;
			}
		}
		if (revertPossible) {
			std::reverse(reverseActions.begin(), reverseActions.end());
			return new ActionBatch(reverseActions, true);
		} else {
			for (auto reverseAction : reverseActions) {
				delete reverseAction;
			}
			return nullptr;
		}
	} else { // Run action w/o generating reverse-action
		for (auto action : actions) {
			action->execute(instance, false);
		}
		return nullptr;
	}
}


} // namespace tstudio
