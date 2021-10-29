#include "NodeModule.hpp"

#include <map>

#include <imgui_internal.h>
#include <torasu/std/Dnum.hpp>
#include <torasu/std/Dstring.hpp>

#include "../../../thirdparty/imnodes/imnodes.h"
#include "../../state/App.hpp"
#include "../../state/TreeManager.hpp"

#define DEBUG_LOG true
#define DO_SNAP false

#if DEBUG_LOG
#include <iostream>
#endif
namespace tstudio {

struct NodeModule::State {
	typedef int node_id;
	struct NodeObj {
		TreeManager::ElementNode* elemNode;
		node_id nodeId;
		std::map<node_id, std::string> attributeIds; 
		std::set<NodeObj*> ownedNodes; 
	};

	node_id idCounter = 0;

	std::map<TreeManager::ElementNode*, NodeObj*> objMap;
	std::map<node_id, NodeObj*> idMap;
	bool needsRemap = true;
	tstudio::TreeManager::version_t treeVersion;

	bool removeNode(TreeManager::ElementNode* node) {
		auto found = objMap.find(node);
		if (found == objMap.end()) return false;
		NodeObj* toRemove = found->second;
		removeNode(toRemove);
		return true;
	}

	void removeNode(NodeObj* toRemove) {
		for (auto attrIds : toRemove->attributeIds) {
			idMap.erase(attrIds.first);
		}
		objMap.erase(toRemove->elemNode);
		idMap.erase(toRemove->nodeId);
		delete toRemove;
	}

	NodeObj* updateNode(TreeManager::ElementNode* node) {
		auto found = objMap.find(node);
		NodeObj* currNode;
		if (found != objMap.end()) {
			currNode = found->second;
		} else {
			currNode = new NodeObj(); 
			currNode->elemNode = node;
			currNode->nodeId = idCounter++;
			// XXX To be more easy viewable
			ImVec2 pos(100.0f, 100.0f+100.0f*objMap.size());
			ImNodes::SetNodeGridSpacePos(currNode->nodeId, pos);
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
		std::set<NodeObj*> nolongerOwned = currNode->ownedNodes;
		for (auto slot : slotMap) {
			// Set attribute-ids
			attributeIds[ids[idIndex]] = slot.first;
			idIndex++;

			// Managed owned nodes
			if (slot.second.ownedByNode) {
				NodeObj* ownedObj = updateNode(slot.second.mounted);
				auto found = nolongerOwned.find(ownedObj);
				if (found != nolongerOwned.end()) { // Already existed before
					nolongerOwned.erase(found);
				} else { // New
					currNode->ownedNodes.insert(ownedObj);
				}
			}
		}
		for (auto* toRemove : nolongerOwned) {
			currNode->ownedNodes.erase(toRemove);
			removeNode(toRemove);
		}

		return currNode;
	}

	~State() {
		for (auto entry : objMap) {
			delete entry.second;
		}
	}

	void mapNode(TreeManager::ElementNode* node) {
		updateNode(node);
	}

	void remap(App* instance) {
		auto* treeManager = instance->getTreeManager();
		auto newTreeVersion = treeManager->getVersion();
		if (!needsRemap && treeVersion == newTreeVersion) return;
#if DEBUG_LOG
		auto bench = std::chrono::steady_clock::now();
		std::cout << "Node-View: Remap..."
			" (" << (needsRemap ? "REINIT - " : "") << std::to_string(treeVersion) << ">>" << std::to_string(newTreeVersion)  << ")"<< std::endl;
#endif
		for (auto* node : treeManager->getManagedNodes()) {
			mapNode(node);
		}

#if DEBUG_LOG
		std::cout << "Node-View: Remap done"
			" (" << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - bench).count() << "µs)"<< std::endl;

#endif
		treeVersion = newTreeVersion;
		needsRemap = false;
	}
};

NodeModule::NodeModule() : state(new State()) {}

NodeModule::~NodeModule() {
	delete state;
}

void NodeModule::onMount() {}

namespace {

void renderDataEditor(TreeManager::ElementNode* node) {
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
		char buffer[buffSize];
		std::copy_n(currString.c_str(), currString.size(), buffer);
		ImGui::PushItemWidth(100.0f);
		if (ImGui::InputText("Value", buffer, buffSize)) {
			node->setModifiedData(new torasu::tstd::Dstring(std::string(buffer)));
		}
		ImGui::PopItemWidth();
	}
}
	
} // namespace

void NodeModule::render(App* instance) {
	state->remap(instance);

	ImNodes::BeginNodeEditor();

	for (auto nodeEntry : state->objMap) {
		TreeManager::ElementNode* node = nodeEntry.first;
		State::NodeObj& nodeIds = *nodeEntry.second;

		ImNodes::BeginNode(nodeIds.nodeId);

		ImNodes::BeginNodeTitleBar();
		ImGui::Checkbox("", &nodeOpen);
		ImGui::SameLine();
		ImGui::Text("%s [%i]", node->getLabel().name, nodeIds.nodeId);
		if (nodeIds.elemNode->isUpdatePending()) {
			ImGui::SameLine();
			State::node_id hoveredNode;
			ImVec2 circlePadding(0.0f, 2.0f);
			float circleSize = 11.0f;
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
		ImNodes::EndNodeTitleBar();

		if (nodeOpen) {
			ImVec2 widthSpacer = {150.0, 0.0};
			ImGui::Dummy(widthSpacer);

			// Data stuff
			renderDataEditor(node);
		}

		
		// Attribute / Slot stuff
		if (!nodeIds.attributeIds.empty()) {
			auto* slots = node->getSlots();
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
				if (ownedByNode) {
					ImNodes::PushColorStyle(
  						ImNodesCol_Pin, IM_COL32(200, 100, 100, 255));
					ImNodes::BeginInputAttribute(attrEntry.first, ImNodesPinShape_QuadFilled);
					ImNodes::PopColorStyle();
				} else {
					ImNodes::BeginInputAttribute(attrEntry.first, ImNodesPinShape_CircleFilled);
				}
				
				if (nodeOpen) {
					if (slotDescriptor != nullptr) {
						ImGui::Text("%s", slotDescriptor->label.name);
						if (slotDescriptor->label.description != nullptr && ImGui::IsItemHovered()) {
							ImGui::SetTooltip("%s", slotDescriptor->label.description);
						}
					} else {
						ImGui::Text("%s", attrEntry.second.c_str());
					}
					ImGui::SameLine();
					if (connectedNode != nullptr) {
						ImGui::Text("[%s]", connectedNode->getLabel().name);
						if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0)) {
							auto foundConnected = state->objMap.find(connectedNode);
							if (foundConnected != state->objMap.end()) {
								selectNode = foundConnected->second->nodeId;
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
#if DEBUG_LOG
		std::cout << "Node-View: Destory link " << linkReciever << std::endl;
#endif
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
#if DEBUG_LOG
		std::cout << "Node-View: Create link " << linkedNode << "-" << linkReciever << std::endl;
#endif
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
