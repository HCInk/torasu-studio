#include "ActionHistoryModule.hpp"

#include <map>
#include <stack>
#include <queue>

#include <imgui.h>
#include <torasu/std/Dnum.hpp>
#include <torasu/std/Dstring.hpp>

#include "../../state/actions/UserActions.hpp"

namespace tstudio {

ActionHistoryModule::ActionHistoryModule() {}
ActionHistoryModule::~ActionHistoryModule() {}

void ActionHistoryModule::render(App* instance) {
	auto* userActions = instance->getUserActions();
	const auto undoStack = userActions->getUndoStack();
	const auto redoStack = userActions->getRedoStack();
	ImGui::Text("Action History (%zu)", (undoStack.size()+redoStack.size()));
	
	const bool undoAvailable = !undoStack.empty();
	ImGui::BeginDisabled(!undoAvailable);
	if (ImGui::Button("Undo") && undoAvailable) {
		userActions->undo(instance);
	}
	ImGui::EndDisabled();
	
	ImGui::SameLine();
	
	const bool redoAvailable = !redoStack.empty();
	ImGui::BeginDisabled(!redoAvailable);
	if (ImGui::Button("Redo") && redoAvailable) {
		userActions->redo(instance);
	}
	ImGui::EndDisabled();
	
	ImGui::SameLine();
	if (ImGui::Button("Clear")) {
		userActions->clearHistory();
	}

	for (auto entry : redoStack) {
		ImGui::Text("<< Undo \"%s\" (%p)", entry.label.c_str(), entry.action);
	}
	for (int64_t i = undoStack.size()-1; i >= 0; i--) {
		auto entry = undoStack[i];
		ImGui::Text(">> %s (%p)", entry.label.c_str(), entry.action);
	}
}


} // namespace tstudio
