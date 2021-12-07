#ifndef SRC_UI_COMPONENTS_SELECTIONMENU_HPP_
#define SRC_UI_COMPONENTS_SELECTIONMENU_HPP_

#include <string>
#include <vector>

#include <imgui.h>
#include <imgui_internal.h>

namespace tstudio {

template<class T> class SelectionMenu {
public:
	using LabelResolver = const char*(*)(T);
	std::vector<T> resultList;
private:
	bool open = false;
	char searchTerm[1024] = {0x00};
	size_t currentSelected = 0;
	bool updateSearch = true;
	const LabelResolver labelResolver;
public:
	SelectionMenu(LabelResolver labelResolver) : labelResolver(labelResolver) {}

	/** @brief Reset state, call this once this menu is hidden */
	inline void reset() {
		if (open) {
			searchTerm[0x00] = 0x00;
			resultList.clear();
			open = false;
		}
	}

	/**
	 * @brief  Check if searchterm has been updated
	 * @retval nullptr if no update to search-term, 
	 * 		a pointer to a null-terminated string if has updated
	 */
	const char* needsSearchUpdate() {
		return updateSearch ? searchTerm : nullptr;
	}

	/** @brief Submit update of resultList-vector according
	 * 		to the new searchTerm got via needsSearchUpdate() */
	void submitSearchUpdate() {
		updateSearch = false;
		currentSelected = 0;
	}

	/**
	 * @brief  Render Menu
	 * @param[out]  selection: Field in which selection will be set
	 * @return true: if selections has been set, false: if nothing has been selected yet
	 */
	bool render(T* selection) {
		bool submitSelection = false;
		bool selectionDone = false;
		if (!open) {
			ImGui::SetKeyboardFocusHere(0);
			open = true;
			updateSearch = true;
		}
		updateSearch |= ImGui::InputText("Search", searchTerm, sizeof(searchTerm)/sizeof(char));
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow), true)) {
			currentSelected++;
			if (currentSelected >= resultList.size()) {
				currentSelected = 0;
			}
		} else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow), true)) {
			if (currentSelected <= 0) {
				currentSelected = resultList.size();
			}
			currentSelected--;
		} else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter), true)) {
			submitSelection = true;
		}

		size_t currItemNum = 0;
		auto* window = ImGui::GetCurrentWindow();
		auto prevWindowFlags = window->Flags;
		window->Flags &= ~ImGuiWindowFlags_Popup;

		for (T listEntry : resultList) {
			bool selected = currItemNum == currentSelected;
			std::string itemLabel = std::string(selected ? ">> " : "") + std::string(labelResolver(listEntry)) + "###" + std::to_string(currItemNum);
			if (!selected) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4({1.0, 1.0, 1.0, 0.6}));
			if (ImGui::MenuItem(itemLabel.c_str()) || (selected && submitSelection)) {
				*selection = listEntry;
				selectionDone = true;
			} else if (ImGui::IsItemHovered()) {
				currentSelected = currItemNum;
			}
			if (!selected) ImGui::PopStyleColor();
			currItemNum++;
		}

		window->Flags = prevWindowFlags;

		return selectionDone;
	}
};

} // namespace tstudio

#endif // SRC_UI_COMPONENTS_SELECTIONMENU_HPP_
