#include "ImageMonitorDisplay.hpp"

#include <imgui.h>

namespace tstudio {

ImageMonitorDisplay::ImageMonitorDisplay(ImageMonitor* imageMon) : imageMon(imageMon) {}

ImageMonitorDisplay::~ImageMonitorDisplay() {}

ImageMonitorDisplay* ImageMonitorDisplay::matchAndCreate(Monitor::MonitorImplementation* impl) {
	if (auto* casted = dynamic_cast<ImageMonitor*>(impl)) {
		return new ImageMonitorDisplay(casted);
	}
	return nullptr;
}

bool ImageMonitorDisplay::match(Monitor::MonitorImplementation* impl) {
	return imageMon == impl && dynamic_cast<ImageMonitor*>(impl) != nullptr;
}

void ImageMonitorDisplay::updateTexture(const tstudio::blank_callbacks& callbacks, bool destroyDisplay) {
	bool destoryTexture = true;
	if (!destroyDisplay) {
		destoryTexture = false;
		auto* currImg = imageMon->getCurrentImage();
		if (currImg != nullptr) {
			if (currImg != loadedTexture) {
				uint32_t texWidth = currImg->getWidth();
				uint32_t texHeight = currImg->getHeight();
				uint8_t* data = currImg->getImageData();
				if (hasTexture) {
					callbacks.update_texture(textureId, texWidth, texHeight, data);
				} else {
					textureId = callbacks.create_texture(texWidth, texHeight, data);
					hasTexture = true;
				}
				texture = callbacks.tex_id_to_imgui_id(textureId);
				texturePending = false;
			}
		} else {
			texturePending = true;
		}
		loadedTexture = currImg;
	}

	if (destoryTexture && hasTexture) {
		callbacks.destory_texture(textureId);
		hasTexture = false;
	}
}

void ImageMonitorDisplay::onBlank(const tstudio::blank_callbacks& callbacks) {
	updateTexture(callbacks, false);
}

void ImageMonitorDisplay::uninit(const tstudio::blank_callbacks& callbacks) {
	updateTexture(callbacks, true);
}

void ImageMonitorDisplay::render() {
	auto max = ImGui::GetContentRegionMax();
	max.y -= 70;
	if (max.y < 0) max.y = 0;
	uint32_t selectedWidth = imageMon->getSelectedWidth();
	uint32_t selectedHeight = imageMon->getSelectedHeight();
	float ratio = static_cast<float>(selectedWidth)/selectedHeight;
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
	if (!texturePending) {
		ImGui::Image(texture, max);
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
	} else {
		ImGui::TextUnformatted("Image pending...");
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
				auto sizeFactor = factors[item_current_idx];
				imageMon->setSize(1920*sizeFactor, 1080*sizeFactor);
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();

	ImGui::SameLine();
	ImGui::Text("%u x %u", selectedWidth, selectedHeight);
};


} // namespace tstudio
