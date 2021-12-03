#include "ElementEditor.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include "../../../thirdparty/imnodes/imnodes.h"

#include <torasu/std/Dnum.hpp>
#include <torasu/std/Dstring.hpp>

#include "Symbols.hpp"

#define DO_SNAP false

namespace tstudio {

void ElementEditor::renderDataEditor(TreeManager::ElementNode* node) {
	auto* currData = node->getCurrentData();
	if (currData == nullptr) return;

	if (node->getType() == "STD::RNUM") {
		auto* numData = dynamic_cast<torasu::tstd::Dnum*>(currData);
		if (numData == nullptr) return;
		float value = numData->getNum();
		ImGui::PushItemWidth(100.0f);
		if (ImGui::DragFloat("Value", &value, 0.01, -30.0f, 30.0f, "%.03f")) {
			(*dynamic_cast<torasu::tstd::Dnum*>(node->getDataForModification())) = value;
		}
		ImGui::PopItemWidth();
	} else if (node->getType() == "STD::RSTRING") {
		auto* strData = dynamic_cast<torasu::tstd::Dstring*>(currData);
		if (strData == nullptr) return;
		const std::string& currString = strData->getString();
		size_t buffSize = currString.size()+1024*10;
		char buffer[buffSize+1];
		std::copy_n(currString.c_str(), currString.size(), buffer);
		buffer[currString.size()] = 0x00;
		ImGui::PushItemWidth(100.0f);
		if (ImGui::InputText("Value", buffer, buffSize)) {
			node->setModifiedData(new torasu::tstd::Dstring(std::string(buffer)));
		}
		ImGui::PopItemWidth();
	}
}

namespace {

using InlineValueNodeRenderer = void(tstudio::TreeManager::ElementNode* node, const std::string& slotLabel, const std::string& slotDescription);

static void InlineNumRenderer(tstudio::TreeManager::ElementNode* node, const std::string& slotLabel, const std::string& slotDescription) {
	auto* currData = node->getCurrentData();
	if (currData != nullptr) if (auto* numData = dynamic_cast<torasu::tstd::Dnum*>(currData)) {
		float value = numData->getNum();
		ImGui::PushItemWidth(100.0f);

		if (ImGui::DragFloat(slotLabel.c_str(), &value, 0.01, -30.0f, 30.0f, "%.03f")) {
			(*dynamic_cast<torasu::tstd::Dnum*>(node->getDataForModification())) = value;
		}

		if (ImGui::IsItemHovered() && !slotDescription.empty()) {
			ImGui::SetTooltip("%s", slotDescription.c_str());
		}
		ImGui::PopItemWidth();
	}
}

static void InlineStringRenderer(tstudio::TreeManager::ElementNode* node, const std::string& slotLabel, const std::string& slotDescription) {
	auto* currData = node->getCurrentData();
	if (currData != nullptr) if (auto* strData = dynamic_cast<torasu::tstd::Dstring*>(currData)) {
		const std::string& currString = strData->getString();
		size_t buffSize = currString.size()+1024*10;
		char buffer[buffSize+1];
		std::copy_n(currString.c_str(), currString.size(), buffer);
		buffer[currString.size()] = 0x00;
		ImGui::PushItemWidth(100.0f);
		
		if (ImGui::InputText(slotLabel.c_str(), buffer, buffSize)) {
			node->setModifiedData(new torasu::tstd::Dstring(std::string(buffer)));
		}

		if (ImGui::IsItemHovered() && !slotDescription.empty()) {
			ImGui::SetTooltip("%s", slotDescription.c_str());
		}
		ImGui::PopItemWidth();
	}
}

static InlineValueNodeRenderer* getInlineValueNodeRenderer(tstudio::TreeManager::ElementNode* node) {
	torasu::Identifier nodeType = node->getType();
	if (nodeType == "STD::RNUM") {
		return InlineNumRenderer;
	} else if (nodeType == "STD::RSTRING") {
		return InlineStringRenderer;
	} else {
		return nullptr;
	}
};

} // namespace


void ElementEditor::renderNodeContents(const NodeDisplayObj& nodeIds, bool nodeOpen, TreeManager::ElementNode** selectNode, NodeModule::State* state, bool renderOutput) {
	if (nodeOpen) {
		ImVec2 widthSpacer = {150.0, 0.0};
		ImGui::Dummy(widthSpacer);

		// Data stuff
		renderDataEditor(nodeIds.elemNode);
	}

	// Attribute / Slot stuff
	if (!nodeIds.attributeIds.empty()) {
		auto* slots = nodeIds.elemNode->getSlots();
		size_t attrIndex = 0;
#if DO_SNAP
		ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkCreationOnSnap);
#endif
		ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
		for (auto attrEntry : nodeIds.attributeIds) {
			TreeManager::ElementNode* connectedNode = nullptr;
			const torasu::ElementFactory::SlotDescriptor* slotDescriptor = nullptr;
			bool ownedByNode = false;
			auto foundSlot = slots->find(attrEntry.second);
			if (foundSlot != slots->end()) {
				connectedNode = foundSlot->second.mounted;
				slotDescriptor = foundSlot->second.descriptor;
				ownedByNode = foundSlot->second.ownedByNode;
			}

			std::string slotLabel;
			std::string slotDescription;
			if (slotDescriptor != nullptr) {
				slotLabel = slotDescriptor->label.name;
				if (slotDescriptor->label.description != nullptr) 
					slotDescription = slotDescriptor->label.description;
			} else {
				slotLabel = attrEntry.second;
			}

			if (ownedByNode) {
				ImGui::PushID(attrIndex);

				InlineValueNodeRenderer* foundInlineRenderer = getInlineValueNodeRenderer(connectedNode);

				if (foundInlineRenderer != nullptr) {

					ImNodes::BeginInputAttribute(attrEntry.first, ImNodesPinShape_CircleFilled);
					if (nodeOpen) {
						foundInlineRenderer(connectedNode, slotLabel, slotDescription);
						if (connectedNode->isUpdatePending()) {
							ImGui::SameLine();
							Symbols::ApplyingIcon();
						}
					}
					ImNodes::EndInputAttribute();

				} else {

					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					ImDrawListSplitter splitter;
					splitter.Split(draw_list, 2);
					// draw_list->ChannelsSplit(2);

					// render group content
					splitter.SetCurrentChannel(draw_list, 1);
					// draw_list->ChannelsSetCurrent(1);
					ImGui::BeginGroup();
					
					if (nodeOpen) {
						ImGui::Dummy(ImVec2(0,2));
						ImGui::Text("%s", slotLabel.c_str());
						if (ImGui::IsItemHovered() && !slotDescription.empty()) {
							ImGui::SetTooltip("%s", slotDescription.c_str());
						}
						ImGui::SameLine();
						ImGui::Text("[%s] #%i", connectedNode->getLabel().name, attrEntry.first);
						if (connectedNode->isUpdatePending()) {
							ImGui::SameLine();
							Symbols::ApplyingIcon();
						}
					}

					NodeDisplayObj* foundNodeObj = nullptr;

					for (auto owned : nodeIds.ownedNodes) {
						if (connectedNode == owned->elemNode) {
							foundNodeObj = owned;
							break;
						}
					}

					if (foundNodeObj != nullptr) {
						renderNodeContents(*foundNodeObj, nodeOpen, selectNode, state, false);
					} else {
						ImGui::TextUnformatted("[Unresolvable!]");
					}

					if (nodeOpen) {
						ImGui::Dummy(ImVec2(0,2));
					}

					// auto cursorPos = window->DC.CursorPos;
					// window->DrawList->Add(ImVec2(cursorPos.x, cursorPos.y), 10, IM_COL32(255, 100, 100, 255));
					ImGui::EndGroup();
					
					// render a background quad
					splitter.SetCurrentChannel(draw_list, 0);
					// draw_list->ChannelsSetCurrent(0);

					if (nodeOpen) {
						auto min = ImGui::GetItemRectMin();
						min.x -= 4;
						auto max = ImGui::GetItemRectMax();
						max.x += 4;
						ImGui::GetWindowDrawList()->AddRectFilled(min, max, IM_COL32(0, 0, 0, 50), 4);
					}

					splitter.Merge(draw_list);
					// draw_list->ChannelsMerge();

				}

				ImGui::PopID();
			} else {
				ImNodes::BeginInputAttribute(attrEntry.first, ImNodesPinShape_CircleFilled);

				if (nodeOpen) {
					ImGui::Text("%s", slotLabel.c_str());
					if (ImGui::IsItemHovered() && !slotDescription.empty()) {
						ImGui::SetTooltip("%s", slotDescription.c_str());
					}
					ImGui::SameLine();
					if (connectedNode != nullptr) {
						ImGui::Text("[%s]", connectedNode->getLabel().name);
						if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0)) {
							*selectNode = connectedNode;
							// auto foundConnected = state->objMap.find(connectedNode);
							// if (foundConnected != state->objMap.end()) {
							// 	*selectNode = foundConnected->second->nodeId;
							// }
						}
					} else {
						ImGui::PushItemWidth(100.0f);
						ImGui::Text("[UNLINKED]");
						// ImGui::DragFloat("", &someFloat, 0.01, -30.0f, 30.0f, "%.03f");
						ImGui::PopItemWidth();
					}
				}
				ImNodes::EndInputAttribute();
			}

			auto size = ImGui::GetItemRectSize();

			if (renderOutput && attrIndex == 0) {
				ImNodes::PopAttributeFlag();
				ImGui::SameLine();
				ImNodes::BeginOutputAttribute(nodeIds.nodeId);
				ImNodes::EndOutputAttribute();
				ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
			}

			if (nodeOpen) {
				float addedHeight = std::max(15.0f-size.y, 0.0f);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY()+addedHeight);
				// std::cout << "size " << nodeId << "-" << i << ": " << " " << addedHeight << std::endl;
			}
			attrIndex++;
		}

		ImNodes::PopAttributeFlag();
#if DO_SNAP
		ImNodes::PopAttributeFlag();
#endif
	} else if (renderOutput) {
		ImNodes::BeginOutputAttribute(nodeIds.nodeId);
		ImNodes::EndOutputAttribute();
	}
}

} // namespace tstudio
