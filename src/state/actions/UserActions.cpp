#include "UserActions.hpp"

#include <memory>

namespace tstudio {

UserActions::UserActions() {
}

UserActions::~UserActions() {
	clearUndo();
}

// Stack Management

void UserActions::pushUndo(UserAction* action) {
	undoStack.push_back(action);
}

UserAction* UserActions::popUndo() {
	if (undoStack.empty()) return nullptr;
	auto lastIt = undoStack.begin() + (undoStack.size()-1);
	UserAction* action = *lastIt;
	undoStack.erase(lastIt);
	return action;
}

void UserActions::clearUndo() {
	for (auto* undoAction : undoStack) {
		delete undoAction;
	}
	undoStack.clear();
}

// Actions

void UserActions::execute(App* instance, UserAction* action) {
	std::unique_ptr<UserAction> actionOwner(action);
	UserAction* undoAction = action->execute(instance, true);
	if (undoAction != nullptr) {
		pushUndo(action);
	} else {
		clearUndo();
	}
}

void UserActions::undo(App* instance) {
	std::unique_ptr<UserAction> action(popUndo());
	if (!action) return;

	action->execute(instance, false /* no redo yet */ );
}


} // namespace tstudio
