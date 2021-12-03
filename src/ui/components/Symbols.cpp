#include "Symbols.hpp"

#include <imgui_internal.h>

namespace tstudio {

void Symbols::ApplyingIcon() {
	ImVec2 circlePadding(0.0f, 2.0f);
	const static float circleSize = 11.0f;
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (!window->SkipItems) {
		auto cursorPos = window->DC.CursorPos; // ImGui::GetCursorPos();
		ImVec2 endPos(cursorPos.x + circleSize + circlePadding.x*2, cursorPos.y + circleSize + circlePadding.y*2);
		ImVec2 center(cursorPos.x + circleSize/2 + circlePadding.x, cursorPos.y + circleSize/2 + circlePadding.y);
		ImRect bb(cursorPos, endPos);
		ImGui::Dummy(ImVec2(circleSize,circleSize));
		// ImGui::ItemSize(bb);
		drawList->AddCircleFilled(center, circleSize/2, IM_COL32(255, 100, 100, 255));
		// drawList->AddCircle(center, circleSize/2, IM_COL32(255, 255, 255, 255), 0, 1.5f);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Applying changes...");
		}
	}
}

} // namespace tstudio
