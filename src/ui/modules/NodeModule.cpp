#include "NodeModule.hpp"

#include "../../../thirdparty/imnodes/imnodes.h"

namespace tstudio {

NodeModule::NodeModule() {}

NodeModule::~NodeModule() {}


void NodeModule::onMount() {
	for (int nodeId = 0; nodeId < 10; nodeId++) {
		ImVec2 position = ImVec2(nodeId*200.0f+100.0f, 100.0f);
		ImNodes::SetNodeGridSpacePos(nodeId, position);
	}
}

void NodeModule::render(App* instance) {
	ImNodes::BeginNodeEditor();

	int attrId = 0;
	for (int nodeId = 0; nodeId < 10; nodeId++) {

		ImNodes::BeginNode(nodeId);

		ImNodes::BeginNodeTitleBar();
		ImGui::Checkbox("", &nodeOpen);
		ImGui::SameLine();
		ImGui::TextUnformatted("Test Node");
		ImNodes::EndNodeTitleBar();

		ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkCreationOnSnap);
		ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
		if (nodeOpen) {
			ImVec2 widthSpacer = {150.0, 0.0};
			ImGui::Dummy(widthSpacer);
		}

		for (int i = 0; i < 4; i++) {
			ImNodes::BeginInputAttribute(attrId++);
			
			if (nodeOpen) {
				ImGui::Text("in %i", i);
				ImGui::SameLine();
				int linkedTo;
				if (isLinked(attrId-1, &linkedTo)) {
					int destNodeId = linkedTo/5;
					ImGui::Text("[LINKED to %i]", destNodeId);
					if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0)) {
						selectNode = destNodeId;
					}
				} else {
					ImGui::PushItemWidth(100.0f);
					ImGui::DragFloat("", &someFloat, 0.01, -30.0f, 30.0f, "%.03f");
					ImGui::PopItemWidth();
				}
			}
			ImNodes::EndInputAttribute();

			auto size = ImGui::GetItemRectSize();

			if (i == 0) {
				ImNodes::PopAttributeFlag();
				ImGui::SameLine();
				ImNodes::BeginOutputAttribute(attrId++);
				ImNodes::EndOutputAttribute();
				ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
			}

			if (nodeOpen) {
				float addedHeight = std::max(15.0f-size.y, 0.0f);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY()+addedHeight);
				// std::cout << "size " << nodeId << "-" << i << ": " << " " << addedHeight << std::endl;
			}
		}

		ImNodes::PopAttributeFlag();
		ImNodes::PopAttributeFlag();

		ImNodes::EndNode();
	}

	for (size_t i = 0; i < links.size(); ++i) {
		const std::pair<int, int> p = links[i];
		ImNodes::Link(i, p.first, p.second);
	}

	ImNodes::MiniMap();
	ImNodes::EndNodeEditor();
	int linkId;
	if (ImNodes::IsLinkDestroyed(&linkId)) {
		links.erase(links.begin()+linkId);
	}
	int start_attr, end_attr;
	if (ImNodes::IsLinkCreated(&start_attr, &end_attr)) {
		links.push_back(std::make_pair(start_attr, end_attr));
	}
	if (selectNode >= 0) {
		ImNodes::ClearNodeSelection();
		ImNodes::SelectNode(selectNode);
		ImVec2 selectedPos = ImNodes::GetNodeGridSpacePos(selectNode);
		selectedPos.x *= -1;
		selectedPos.y *= -1;
		selectedPos.x += 200.0;
		selectedPos.y += 200.0;
		ImNodes::EditorContextResetPanning(selectedPos);
		selectNode = -1;
	}
}

bool NodeModule::isLinked(int attrId, int* otherLink) {
	for (auto link : links) {
		if (link.first == attrId) {
			*otherLink = link.second;
			return true;
		}
		if (link.second == attrId) {
			*otherLink = link.first;
			return true;
		}
	}
	return false;
}

} // namespace tstudio
