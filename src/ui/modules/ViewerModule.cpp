#include "ViewerModule.hpp"

#include <imgui.h>

namespace tstudio {

ViewerModule::ViewerModule(ViewerState* stateRef) 
	: stateRef(stateRef) {}

ViewerModule::~ViewerModule() {}

void ViewerModule::render(App* instance) {
	ImGui::Text("Hello from another window!");
	auto max = ImGui::GetContentRegionMax();
	max.y -= 70;
	if (max.y < 0) max.y = 0;
	float ratio = static_cast<float>(stateRef->texWidth)/stateRef->texHeight;
	float maxWidth = max.y * ratio;
	float maxHeight = max.x / ratio;
	float sidePadding = 0;
	if (maxWidth <= max.x) {
		sidePadding = (max.x-maxWidth)/2;
		max.x = maxWidth;
	} else {
		max.y = maxHeight;
	}
	ImGui::SetCursorPosX(ImGui::GetCursorPosX()+sidePadding);
	ImGui::Image(stateRef->image_texture, max);
	if (!ImGui::IsWindowDocked()) {
		if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home))) {
			ImVec2 veiwportSize = ImGui::GetMainViewport()->Size;
			ImVec2 defPos = {veiwportSize.x*1/3, veiwportSize.y*1/3};
			ImVec2 defSize = {veiwportSize.x*1/3, veiwportSize.y*1/3};
			ImGui::SetWindowPos(defPos);
			ImGui::SetWindowSize(defSize);
		}
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			auto mousePos = ImGui::GetMousePos();
			auto windowPos = ImGui::GetWindowPos();
			auto windowSize = ImGui::GetWindowSize();
			ImVec2 posRel = {
				(mousePos.x-windowPos.x)/windowSize.x,
				(mousePos.y-windowPos.y)/windowSize.y,
			};

			static const float zoomFactor = 1.5;

			float currZoomFactor;
			
			if (ImGui::GetIO().KeyShift) {
				currZoomFactor = zoomFactor;
			} else {
				currZoomFactor = 1/zoomFactor;
			}

			const ImVec2 toAdd = {windowSize.x*(currZoomFactor-1), windowSize.y*(currZoomFactor-1)};

			windowPos.x -= toAdd.x*posRel.x;
			windowPos.y -= toAdd.y*posRel.y;
			windowSize.x += toAdd.x;
			windowSize.y += toAdd.y;

			if (windowSize.x > 100 && windowSize.y > 150) {
				ImGui::SetWindowPos(windowPos);
				ImGui::SetWindowSize(windowSize);
			}

		}

	}

	// if (ImGui::Button("Close Me"))
	// 	show_another_window = false;

	// ImGui::SameLine();

	const char* items[] = { "scale x0.25", "scale x0.5", "scale x1", "scale x2", "scale x4"};
	float factors[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
	static int item_current_idx = 2; // Here we store our selection data as an index.
	const char* combo_preview_value = items[item_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)
	ImGui::PushItemWidth(100.0f);
	if (ImGui::BeginCombo(" ", combo_preview_value))
	{
		for (int n = 0; n < IM_ARRAYSIZE(items); n++)
		{
			const bool is_selected = (item_current_idx == n);
			if (ImGui::Selectable(items[n], is_selected)) {
				item_current_idx = n;
				stateRef->sizeFactor = factors[item_current_idx];
				stateRef->texWidth = stateRef->baseTexWidth*stateRef->sizeFactor;
				stateRef->texHeight = stateRef->baseTexHeight*stateRef->sizeFactor;
				stateRef->reloadTexture = true;
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();

	ImGui::SameLine();
	ImGui::Text("%u x %u", stateRef->texWidth, stateRef->texHeight);

}

} // namespace tstudio
