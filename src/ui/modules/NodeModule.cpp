#include "NodeModule.hpp"

#include <map>
// #include <iostream>

#include "../../../thirdparty/imnodes/imnodes.h"
#include "../../state/App.hpp"
#include "../../state/TreeManager.hpp"

#define DO_SNAP false
namespace tstudio {

struct NodeModule::State {
	typedef int node_id;
	struct NodeObj {
		TreeManager::ElementNode* elemNode;
		node_id nodeId;
		std::map<node_id, std::string> attributeIds; 
	};

	node_id idCounter = 0;

	std::map<TreeManager::ElementNode*, NodeObj*> objMap;
	std::map<node_id, NodeObj*> idMap;
	bool needsRemap = true;

	bool removeNode(TreeManager::ElementNode* node) {
		auto found = objMap.find(node);
		if (found == objMap.end()) return false;
		NodeObj* toRemove = found->second;
		for (auto attrIds : toRemove->attributeIds) {
			idMap.erase(attrIds.first);
		}
		idMap.erase(toRemove->nodeId);
		delete toRemove;
		return true;
	}

	void updateNode(TreeManager::ElementNode* node) {
		auto found = objMap.find(node);
		NodeObj* currNode;
		if (found != objMap.end()) {
			currNode = found->second;
		} else {
			currNode = new NodeObj(); 
			currNode->elemNode = node;
			currNode->nodeId = idCounter++;
			objMap[node] = currNode;
			idMap[currNode->nodeId] = currNode;
		}

		// Adjusting attributes

		auto& attributeIds = currNode->attributeIds;
		std::vector<node_id> ids;
		for (auto idEntry : attributeIds) {
			ids.push_back(idEntry.first);
		}

		auto& slotMap = *node->getSlots();
		size_t neededSize = slotMap.size();
		int32_t sizeDelta = neededSize-ids.size();
		if (sizeDelta < 0) { // shrink
			for (size_t i = ids.size(); i > neededSize; ) {
				i--;
				node_id toErase = ids[i];
				idMap.erase(toErase);
				attributeIds.erase(toErase);
			}
		} else { // grow
			for (size_t i = 0; i < sizeDelta; i++) {
				node_id newId = idCounter++;
				idMap[newId] = currNode;
				ids.push_back(newId);
			}
		}

		size_t idIndex = 0;
		for (auto slot : slotMap) {
			attributeIds[ids[idIndex]] = slot.first;
			idIndex++;
		}
	}

	~State() {
		for (auto entry : objMap) {
			delete entry.second;
		}
	}

	void mapNode(TreeManager::ElementNode* node) {
		updateNode(node);
		for (auto slot : *node->getSlots()) {
			if (slot.second.ownedByNode) {
				updateNode(slot.second.mounted);
			}
		}
	}

	void remap(App* instance) {
		if (!needsRemap) return;

		auto* treeManager = instance->getTreeManager();

		for (auto* node : treeManager->getManagedNodes()) {
			mapNode(node);
		}

		needsRemap = false;
	}
};

NodeModule::NodeModule() : state(new State()) {}

NodeModule::~NodeModule() {
	delete state;
}

void NodeModule::onMount() {}

void NodeModule::render(App* instance) {
	state->remap(instance);

	ImNodes::BeginNodeEditor();

	for (auto nodeEntry : state->objMap) {
		auto* node = nodeEntry.first;
		auto& nodeIds = *nodeEntry.second;

		ImNodes::BeginNode(nodeIds.nodeId);

		ImNodes::BeginNodeTitleBar();
		ImGui::Checkbox("", &nodeOpen);
		ImGui::SameLine();
		ImGui::TextUnformatted(node->getLabel().name);
		ImNodes::EndNodeTitleBar();

		if (nodeOpen) {
			ImVec2 widthSpacer = {150.0, 0.0};
			ImGui::Dummy(widthSpacer);
		}

		if (!nodeIds.attributeIds.empty()) {
			auto* slots = node->getSlots();
			size_t attrIndex = 0;
#if DO_SNAP
			ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkCreationOnSnap);
#endif
			ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
			for (auto attrEntry : nodeIds.attributeIds) {
				State::node_id connectedNodeId = -1;
				TreeManager::ElementNode* connectedNode = nullptr;
				const torasu::ElementFactory::SlotDescriptor* slotDescriptor = nullptr;
				auto foundSlot = slots->find(attrEntry.second);
				if (foundSlot != slots->end()) {
					connectedNode = foundSlot->second.mounted;
					slotDescriptor = foundSlot->second.descriptor;
					
					auto found = state->objMap.find(connectedNode);
					if (found != state->objMap.end()) {
						connectedNodeId = found->second->nodeId;
					}
				}
				ImNodes::BeginInputAttribute(attrEntry.first);
				
				if (nodeOpen) {
					ImGui::Text(slotDescriptor->label.name);
					ImGui::SameLine();
					if (connectedNode != nullptr) {
						ImGui::Text("[LINKED to %s]", connectedNode->getLabel().name);
						if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0)) {
							if (connectedNodeId >= 0) {
								selectNode = connectedNodeId;
							}
						}
					} else {
						ImGui::PushItemWidth(100.0f);
						ImGui::Text("[UNLINKED]");
						// ImGui::DragFloat("", &someFloat, 0.01, -30.0f, 30.0f, "%.03f");
						ImGui::PopItemWidth();
					}
				}
				ImNodes::EndInputAttribute();

				auto size = ImGui::GetItemRectSize();

				if (attrIndex == 0) {
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
		} else {
			ImNodes::BeginOutputAttribute(nodeIds.nodeId);
			ImNodes::EndOutputAttribute();
		}


		ImNodes::EndNode();
	}


	for (auto nodeEntry : state->objMap) {
		auto* slots = nodeEntry.first->getSlots();

		for (auto attrEntry : nodeEntry.second->attributeIds) {
			State::node_id connectedNodeId = -1;
			
			auto foundSlot = slots->find(attrEntry.second);
			if (foundSlot != slots->end() && foundSlot->second.mounted != nullptr) {
				auto found = state->objMap.find(foundSlot->second.mounted);
				if (found != state->objMap.end()) {
					connectedNodeId = found->second->nodeId;
				}
			}
			if (connectedNodeId >= 0) {
				ImNodes::Link(attrEntry.first, connectedNodeId, attrEntry.first);
			}
		}
	}

	ImNodes::MiniMap();
	ImNodes::EndNodeEditor();
	ImVec2 editorSize = ImGui::GetItemRectSize();

	State::node_id linkReciever;
	if (ImNodes::IsLinkDestroyed(&linkReciever)) {
		// std::cout << "Destory " << linkReciever << std::endl;
		auto foundNode = state->idMap.find(linkReciever);
		if (foundNode != state->idMap.end()) {
			auto foundAttr = foundNode->second->attributeIds.find(linkReciever);
			if (foundAttr != foundNode->second->attributeIds.end()) {
				foundNode->second->elemNode->putSlot(foundAttr->second.c_str(), nullptr);
			} else {
				throw std::logic_error("Error resolving atrribute in node which is mapped for that id");
			}
		} else {
			throw std::logic_error("Failed to find node for link-id");
		}
	}
	State::node_id linkedNode;
	if (ImNodes::IsLinkCreated(&linkedNode, &linkReciever)) {
		// std::cout << "Create " << linkedNode << "-" << linkReciever << std::endl;
		auto foundLinked = state->idMap.find(linkedNode);
		auto foundNode = state->idMap.find(linkReciever);
		if (foundLinked != state->idMap.end() || foundNode != state->idMap.end()) {
			auto foundAttr = foundNode->second->attributeIds.find(linkReciever);
			if (foundAttr != foundNode->second->attributeIds.end()) {
				foundNode->second->elemNode->putSlot(foundAttr->second.c_str(), foundLinked->second->elemNode);
			} else {
				throw std::logic_error("Error resolving atrribute in node which is mapped for that id");
			}
		} else {
			throw std::logic_error("Failed to find node(s) for link-id");
		}
		// links.push_back(std::make_pair(start_attr, end_attr));
	}
	if (selectNode >= 0) {
		ImNodes::ClearNodeSelection();
		ImNodes::SelectNode(selectNode);
		ImVec2 selectedPos = ImNodes::GetNodeGridSpacePos(selectNode);
		selectedPos.x *= -1;
		selectedPos.y *= -1;
		selectedPos.x += editorSize.x/2-70;
		selectedPos.y += editorSize.y/2-50;
		ImNodes::EditorContextResetPanning(selectedPos);
		selectNode = -1;
	}
}

} // namespace tstudio
