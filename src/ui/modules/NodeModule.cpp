#include "NodeModule.hpp"

#include <map>
#include <stack>
#include <queue>

#include <imgui_internal.h>
#include <torasu/std/Dnum.hpp>
#include <torasu/std/Dstring.hpp>

#include "../../../thirdparty/imnodes/imnodes.h"
#include "../../state/App.hpp"
#include "../../state/TreeManager.hpp"
#include "../../state/ElementDisplay.hpp"

#define DEBUG_LOG true
#define DO_SNAP false

#if DEBUG_LOG
#include <iostream>
#endif
namespace tstudio {

static void ApplyingIcon() {
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

struct NodeModule::State {
	typedef int node_id;
	struct NodeObj {
		TreeManager::ElementNode* elemNode;
		node_id nodeId;
		std::map<node_id, std::string> attributeIds; 
		std::set<NodeObj*> ownedNodes; 
	};

	node_id idCounter = 0;

	std::map<TreeManager::ElementNode*, NodeObj*> objMap; // init/update on remap
	std::map<node_id, NodeObj*> idMap; // init/update on remap
	bool needsRemap = true;
	tstudio::TreeManager::version_t treeVersion; // init/update on remap
	TreeManager::OutputNode* outputNode; // init/update on remap
	NodeObj* linkedToOutput; // init/update on remap
	const node_id outputId = idCounter++;

	bool removeNode(TreeManager::ElementNode* node) {
		auto found = objMap.find(node);
		if (found == objMap.end()) return false;
		NodeObj* toRemove = found->second;
		removeNode(toRemove);
		return true;
	}

	void removeNode(NodeObj* toRemove) {
		for (NodeObj* owned : toRemove->ownedNodes) {
			removeNode(owned);
		}
		for (auto attrIds : toRemove->attributeIds) {
			idMap.erase(attrIds.first);
		}
		objMap.erase(toRemove->elemNode);
		idMap.erase(toRemove->nodeId);
		delete toRemove;
	}

	void updateNodeContent(NodeObj* node) {
		// Adjusting attributes
		auto& attributeIds = node->attributeIds;
		std::vector<node_id> ids;
		for (auto idEntry : attributeIds) {
			ids.push_back(idEntry.first);
		}

		auto& slotMap = *node->elemNode->getSlots();
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
				idMap[newId] = node;
				ids.push_back(newId);
			}
		}

		size_t idIndex = 0;
		std::set<NodeObj*> unmatchedOwned = node->ownedNodes;
		for (auto slot : slotMap) {
			// Set attribute-ids
			attributeIds[ids[idIndex]] = slot.first;
			idIndex++;

			// Managed owned nodes
			if (slot.second.ownedByNode) {
				NodeObj* ownedObj = nullptr;
				for (NodeObj* currOwned : unmatchedOwned) {
					if (slot.second.mounted == currOwned->elemNode) {
						ownedObj = currOwned;
						break;
					}
				}
				if (ownedObj != nullptr) { // Already existed before
					unmatchedOwned.erase(ownedObj);
				} else { // New
					ownedObj = new NodeObj();
					ownedObj->elemNode = slot.second.mounted;
					ownedObj->nodeId = idCounter++;
					node->ownedNodes.insert(ownedObj);
				}
				updateNodeContent(ownedObj);
			}
		}
		for (auto* toRemove : unmatchedOwned) { // Remove all left owned nodes, which couldn't be matched
			node->ownedNodes.erase(toRemove);
			removeNode(toRemove);
		}
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

		updateNodeContent(currNode);

		return currNode;
	}

	void setLinkedToOutput(NodeObj* nodeObj) {
		linkedToOutput = nodeObj;
		if (nodeObj != nullptr) {
			outputNode->setSelected(nodeObj->elemNode);
		} else {
			outputNode->setSelected(nullptr);
		}
	}

	void resolveUnmountedChildren(NodeModule::State::NodeObj* obj, std::queue<NodeModule::State::NodeObj*>* queueToFill) {
		for (auto& slot : *obj->elemNode->getSlots()) {
			if (slot.second.mounted == nullptr) continue;

			if (slot.second.ownedByNode) {
				for (auto* owned : obj->ownedNodes) {
					if (owned->elemNode == slot.second.mounted) {
						resolveUnmountedChildren(owned, queueToFill);
						break;
					}
				}
			} else {
				auto found = objMap.find(slot.second.mounted);
				if (found != objMap.end()) {
					queueToFill->push(found->second);
				} else {
					throw std::logic_error("Error resolving non-owned element in resolveUnmountedChildren");
				}
			}
		}
	}

	void alignNodes(NodeModule::State::NodeObj* objToAlign) {
		struct ProcLevel {
			NodeModule::State::NodeObj* obj;
			std::queue<NodeModule::State::NodeObj*> childQueue;
			std::vector<std::pair<NodeModule::State::NodeObj*, ImVec2>> positioned;
			uint64_t totalHeight;
			ElementDisplay* displaySettings;
			ElementDisplay::NodeSize nodeSize;
			uint64_t horPos;
			ProcLevel(NodeModule::State::NodeObj* obj, State* state, size_t horPosOff) 
				: obj(obj), totalHeight(0), displaySettings(obj->elemNode->getDisplaySettings()), 
					nodeSize(displaySettings->getNodeSize()), horPos(horPosOff + (nodeSize.hasSize ? nodeSize.width : 100)) {
				state->resolveUnmountedChildren(obj, &childQueue);
			}
		};
		std::stack<ProcLevel> procStack;
		
		procStack.push(ProcLevel(objToAlign, this, 0));

		static const float VERT_PADDING = 10;
		for (;;) {
			ProcLevel& currLevel = procStack.top();
			if (currLevel.childQueue.empty()) {
				ProcLevel levelCpy = currLevel;
				procStack.pop();
				float nodeOffset = VERT_PADDING;
				float ownOffset = 0;
				{ // Finishing up current level
					uint64_t ownHeight = levelCpy.nodeSize.hasSize ? levelCpy.nodeSize.height : 100;
					if (ownHeight > levelCpy.totalHeight) {
						float childOffset = (ownHeight-levelCpy.totalHeight)/2;
						nodeOffset += childOffset;
						ownOffset -= childOffset;
						levelCpy.totalHeight = ownHeight;
					} else {
						ownOffset = (levelCpy.totalHeight-ownHeight)/2;
					}
					levelCpy.positioned.push_back(std::pair<NodeModule::State::NodeObj*, ImVec2>(levelCpy.obj, {-static_cast<float>(levelCpy.horPos), ownOffset}));
				}
				if (procStack.empty()) {
					// Finish up - Apply positions
					for (auto positioned : levelCpy.positioned) {
						positioned.second.y -= ownOffset;
						ImNodes::SetNodeGridSpacePos(positioned.first->nodeId, positioned.second);
						auto* dispSettings = positioned.first->elemNode->getDisplaySettings();
						dispSettings->setNodePosition({
							.mode = ElementDisplay::NodePosition::POS_MODE_SET,
							.x = positioned.second.x,
							.y = positioned.second.y
						});
						dispSettings->reposInProgress = false;
					}
					break;
				}
				// Copying results into parent level
				ProcLevel& aboveLevel = procStack.top();
				nodeOffset += aboveLevel.totalHeight;
				for (auto positioned : levelCpy.positioned) {
					positioned.second.y += nodeOffset;
					aboveLevel.positioned.push_back(positioned);
				}
				aboveLevel.totalHeight += levelCpy.totalHeight+VERT_PADDING*2;
			} else {
				// Recurse further in
				NodeObj* next = currLevel.childQueue.front();
				auto& reposInProgress = next->elemNode->getDisplaySettings()->reposInProgress;
				if (!reposInProgress) { // Only eval yet unpositioned elements
					procStack.push(ProcLevel(next, this, currLevel.horPos+80));
					reposInProgress = true;
				}
				currLevel.childQueue.pop();
			}
		}
	}

	~State() {
		for (auto entry : objMap) {
			removeNode(entry.second);
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
		outputNode = treeManager->getOutputNode();
		TreeManager::ElementNode* selected = outputNode->getSelected();
		if (selected != nullptr) {
			auto found = objMap.find(selected);
			if (found != objMap.end()) {
				linkedToOutput = found->second;
			} else {
				linkedToOutput = nullptr;
				throw std::logic_error("Failed to find node referenced in root");
			}
		} else {
			linkedToOutput = nullptr;
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

void renderNodeContents(const NodeModule::State::NodeObj& nodeIds, bool nodeOpen, int* selectNode, NodeModule::State* state, bool renderOutput) {
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

				if (connectedNode->getType() == "STD::RNUM") {

					ImNodes::BeginInputAttribute(attrEntry.first, ImNodesPinShape_CircleFilled);
					if (nodeOpen) {
						auto* currData = connectedNode->getCurrentData();
						if (currData != nullptr) if (auto* numData = dynamic_cast<torasu::tstd::Dnum*>(currData)) {
							float value = numData->getNum();
							ImGui::PushItemWidth(100.0f);

							if (ImGui::DragFloat(slotLabel.c_str(), &value, 0.01, -30.0f, 30.0f, "%.03f")) {
								(*dynamic_cast<torasu::tstd::Dnum*>(connectedNode->getDataForModification())) = value;
							}

							if (ImGui::IsItemHovered() && !slotDescription.empty()) {
								ImGui::SetTooltip("%s", slotDescription.c_str());
							}
							ImGui::PopItemWidth();
						}
						if (connectedNode->isUpdatePending()) {
							ImGui::SameLine();
							ApplyingIcon();
						}
					}
					ImNodes::EndInputAttribute();

				}  else if (connectedNode->getType() == "STD::RSTRING") {

					ImNodes::BeginInputAttribute(attrEntry.first, ImNodesPinShape_CircleFilled);
					if (nodeOpen) {
						auto* currData = connectedNode->getCurrentData();
						if (currData != nullptr) if (auto* strData = dynamic_cast<torasu::tstd::Dstring*>(currData)) {
							const std::string& currString = strData->getString();
							size_t buffSize = currString.size()+1024*10;
							char buffer[buffSize+1];
							std::copy_n(currString.c_str(), currString.size(), buffer);
							buffer[currString.size()] = 0x00;
							ImGui::PushItemWidth(100.0f);
							
							if (ImGui::InputText(slotLabel.c_str(), buffer, buffSize)) {
								connectedNode->setModifiedData(new torasu::tstd::Dstring(std::string(buffer)));
							}

							if (ImGui::IsItemHovered() && !slotDescription.empty()) {
								ImGui::SetTooltip("%s", slotDescription.c_str());
							}
							ImGui::PopItemWidth();
						}
						if (connectedNode->isUpdatePending()) {
							ImGui::SameLine();
							ApplyingIcon();
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
							ApplyingIcon();
						}
					}

					NodeModule::State::NodeObj* foundNodeObj = nullptr;

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
							auto foundConnected = state->objMap.find(connectedNode);
							if (foundConnected != state->objMap.end()) {
								*selectNode = foundConnected->second->nodeId;
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

void renderLinks(const NodeModule::State::NodeObj& nodeObj, NodeModule::State* state) {
	auto* slots = nodeObj.elemNode->getSlots(); 

	for (auto attrEntry : nodeObj.attributeIds) {
		NodeModule::State::State::node_id connectedNodeId = -1;
		
		auto foundSlot = slots->find(attrEntry.second);
		if (foundSlot != slots->end() && foundSlot->second.mounted != nullptr && !foundSlot->second.ownedByNode) {
			auto found = state->objMap.find(foundSlot->second.mounted);
			if (found != state->objMap.end()) {
				connectedNodeId = found->second->nodeId;
			}
		}
		if (connectedNodeId >= 0) {
			ImNodes::Link(attrEntry.first, connectedNodeId, attrEntry.first);
		}
	}

	for (NodeModule::State::NodeObj* owned : nodeObj.ownedNodes) {
		renderLinks(*owned, state);
	}
}

} // namespace

void NodeModule::render(App* instance) {
	state->remap(instance);

	if (state->linkedToOutput != nullptr 
		&& state->linkedToOutput->elemNode->getDisplaySettings()->getNodePosition().mode != ElementDisplay::NodePosition::POS_MODE_SET
		&& state->linkedToOutput->elemNode->getDisplaySettings()->hasNodeSize()) {
			state->alignNodes(state->linkedToOutput);
	}

	int selectNode = -1;

	ImNodes::BeginNodeEditor();

	for (auto nodeEntry : state->objMap) {
		TreeManager::ElementNode* node = nodeEntry.first;
		State::NodeObj& nodeIds = *nodeEntry.second;

		ImNodes::BeginNode(nodeIds.nodeId);

		ImNodes::BeginNodeTitleBar();
		const bool originalNodeOpen = !node->getDisplaySettings()->doCollapseNode();
		bool nodeOpen = originalNodeOpen;
		ImGui::Checkbox("", &nodeOpen);
		if (nodeOpen != originalNodeOpen) {
			node->getDisplaySettings()->setCollapseNode(!nodeOpen);
		}
		ImGui::SameLine();
		ImGui::Text("%s #%i", node->getLabel().name, nodeIds.nodeId);
		if (nodeIds.elemNode->isUpdatePending()) {
			ImGui::SameLine();
			ApplyingIcon();
		}
		ImNodes::EndNodeTitleBar();

		renderNodeContents(nodeIds, nodeOpen, &selectNode, state, true);

		ImNodes::EndNode();

		{ // Set Position if not recorded yet
			auto* display = node->getDisplaySettings();
			if (!display->hasNodeSize()) {
				auto itemSize = ImGui::GetItemRectSize();
				display->setNodeSize(itemSize.x+20, itemSize.y+20);
			}
		}
	}

	{ // Output Node
		ImNodes::BeginNode(state->outputId);
		ImNodes::BeginNodeTitleBar();
		ImGui::Text("Output #%i", state->outputId);
		if (state->outputNode->hasUpdatePending()) {
			ImGui::SameLine();
			ApplyingIcon();
		}
		ImNodes::EndNodeTitleBar();
		ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
		ImNodes::BeginInputAttribute(state->outputId);
		ImGui::TextUnformatted("master");
		ImNodes::EndInputAttribute();
		ImNodes::PopAttributeFlag();
		ImNodes::EndNode();
	}

	for (auto nodeEntry : state->objMap) {
		renderLinks(*nodeEntry.second, state);
	}

	if (state->linkedToOutput != nullptr) {
		ImNodes::Link(state->outputId, state->linkedToOutput->nodeId, state->outputId);
	}

	ImNodes::MiniMap();
	ImNodes::EndNodeEditor();
	ImVec2 editorSize = ImGui::GetItemRectSize();

	State::node_id linkReciever;
	if (ImNodes::IsLinkDestroyed(&linkReciever)) {
#if DEBUG_LOG
		std::cout << "Node-View: Destory link " << linkReciever << std::endl;
#endif
		if (linkReciever == state->outputId) {
			state->setLinkedToOutput(nullptr);
		} else {
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
	}
	State::node_id linkedNode;
	if (ImNodes::IsLinkCreated(&linkedNode, &linkReciever)) {
#if DEBUG_LOG
		std::cout << "Node-View: Create link " << linkedNode << "-" << linkReciever << std::endl;
#endif
		if (linkReciever == state->outputId) {
			auto foundLinked = state->idMap.find(linkedNode);
			if (foundLinked != state->idMap.end()) {
				state->setLinkedToOutput(foundLinked->second);
			} else {
				throw std::logic_error("Failed to find node for link-id (output-link)");
			}
		} else {
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
