#include "UserActions.hpp"

#include <memory>
#include <sstream>

namespace tstudio {

UserActions::UserActions() {
}

UserActions::~UserActions() {
	clearHistory();
}

// Stack Management

void UserActions::pushUndo(HistoryEntry action) {
	undoStack.push_back(action);
}

UserActions::HistoryEntry UserActions::popUndo() {
	if (undoStack.empty()) throw std::logic_error("Can't pop from empty undo-stack!");
	auto lastIt = undoStack.begin() + (undoStack.size()-1);
	HistoryEntry action = *lastIt;
	undoStack.erase(lastIt);
	return action;
}

void UserActions::clearUndo() {
	for (auto entry : undoStack) {
		delete entry.action;
	}
	undoStack.clear();
}

void UserActions::clearUndoUntil(int64_t indexUntil) {
	for (int64_t i = 0; i < indexUntil; i++) {
		auto entry = undoStack[i];
		delete entry.action;
	}
	undoStack.erase(undoStack.begin(), undoStack.begin()+indexUntil+1);
}

void UserActions::pushRedo(HistoryEntry action) {
	redoStack.push_back(action);
}

UserActions::HistoryEntry UserActions::popRedo() {
	if (redoStack.empty()) throw std::logic_error("Can't pop from empty redo-stack!");
	auto lastIt = redoStack.begin() + (redoStack.size()-1);
	HistoryEntry action = *lastIt;
	redoStack.erase(lastIt);
	return action;
}

void UserActions::clearRedo() {
	for (auto entry : redoStack) {
		delete entry.action;
	}
	redoStack.clear();
}

void UserActions::clearRedoUntil(int64_t indexUntil) {
	for (int64_t i = 0; i < indexUntil; i++) {
		auto entry = redoStack[i];
		delete entry.action;
	}
	redoStack.erase(redoStack.begin(), redoStack.begin()+indexUntil+1);
}


// Actions

void UserActions::execute(App* instance, UserAction* action, std::string label) {
	std::unique_ptr<UserAction> actionOwner(action);
	UserAction* undoAction = action->execute(instance, true);
	clearRedo();
	if (undoAction != nullptr) {
		pushUndo({
			.action = undoAction,
			.label = label,
		});
	} else {
		clearUndo();
	}
}

void UserActions::undo(App* instance) {
	const auto& entry = popUndo();
	std::unique_ptr<UserAction> action(entry.action);
	if (!action) return;

	UserAction* redoAction = action->execute(instance, true);
	if (redoAction != nullptr) {
		pushRedo({
			.action = redoAction,
			.label = entry.label,
		});
	} else {
		clearRedo();
	}
}

void UserActions::redo(App* instance) {
	const auto& entry = popRedo();
	std::unique_ptr<UserAction> action(entry.action);
	if (!action) return;

	UserAction* undoAction = action->execute(instance, true);
	if (undoAction != nullptr) {
		pushUndo({
			.action = undoAction,
			.label = entry.label,
		});
	} else {
		clearUndo();
	}
}

void UserActions::clearHistory() {
	clearUndo();
	clearRedo();
}

void UserActions::notifyDependencyRemoval(App* instance, void* removed) {
	for (int64_t i = undoStack.size()-1; i >= 0; i--) {
		auto entry = undoStack[i];
		void* depArr[] = {removed};
		auto updateResult = entry.action->notifyDependencyRemoval(instance, depArr, 1);
		if (updateResult == UserAction::DependncyUpdateResult::UNAVAILABLE) {
			clearUndoUntil(i);
			break;
		}
	}

	for (int64_t i = redoStack.size()-1; i >= 0; i--) {
		auto entry = redoStack[i];
		void* depArr[] = {removed};
		auto updateResult = entry.action->notifyDependencyRemoval(instance, depArr, 1);
		if (updateResult == UserAction::DependncyUpdateResult::UNAVAILABLE) {
			clearRedoUntil(i);
			break;
		}
	}
}


} // namespace tstudio
