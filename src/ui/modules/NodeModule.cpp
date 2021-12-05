#include "NodeModule.hpp"

#include <map>
#include <stack>
#include <queue>

#include <imgui.h>
#include <torasu/std/Dnum.hpp>
#include <torasu/std/Dstring.hpp>

#include "../../../thirdparty/imnodes/imnodes.h"
#include "../../state/App.hpp"
#include "../../state/TreeManager.hpp"
#include "../../state/ElementDisplay.hpp"
#include "../../state/ElementIndex.hpp"
#include "../components/Symbols.hpp"
#include "../components/NodeDisplayObj.hpp"
#include "../components/ElementEditor.hpp"

#define DEBUG_LOG true

#if DEBUG_LOG
#include <iostream>
#endif

namespace tstudio {

struct NodeModule::State {

	NodeDisplayObj::node_id idCounter = 0;

	std::map<TreeManager::ElementNode*, NodeDisplayObj*> objMap; // init/update on remap
	std::map<NodeDisplayObj::node_id, NodeDisplayObj*> idMap; // init/update on remap
	bool needsRemap = true;
	tstudio::TreeManager::version_t treeVersion; // init/update on remap
	TreeManager::OutputNode* outputNode; // init/update on remap
	NodeDisplayObj* linkedToOutput; // init/update on remap
	const NodeDisplayObj::node_id outputId = idCounter++;

	ImNodesEditorContext* imnodesEditorContext;

	State() : imnodesEditorContext(ImNodes::EditorContextCreate()) {}

	~State() {
		for (auto entry : objMap) {
			removeNode(entry.second);
		}
		ImNodes::EditorContextFree(imnodesEditorContext);
	}

	void removeNode(NodeDisplayObj* toRemove) {
		for (NodeDisplayObj* owned : toRemove->ownedNodes) {
			removeNode(owned);
		}
		for (auto attrIds : toRemove->attributeIds) {
			idMap.erase(attrIds.first);
		}
		objMap.erase(toRemove->elemNode);
		idMap.erase(toRemove->nodeId);
		delete toRemove;
	}

	void updateNodeContent(NodeDisplayObj* node) {
		// Adjusting attributes
		auto& attributeIds = node->attributeIds;
		std::vector<NodeDisplayObj::node_id> ids;
		for (auto idEntry : attributeIds) {
			ids.push_back(idEntry.first);
		}

		auto& slotMap = *node->elemNode->getSlots();
		size_t neededSize = slotMap.size();
		int32_t sizeDelta = neededSize-ids.size();
		if (sizeDelta < 0) { // shrink
			for (size_t i = ids.size(); i > neededSize; ) {
				i--;
				NodeDisplayObj::node_id toErase = ids[i];
				idMap.erase(toErase);
				attributeIds.erase(toErase);
			}
		} else { // grow
			for (size_t i = 0; i < sizeDelta; i++) {
				NodeDisplayObj::node_id newId = idCounter++;
				idMap[newId] = node;
				ids.push_back(newId);
			}
		}

		size_t idIndex = 0;
		std::set<NodeDisplayObj*> unmatchedOwned = node->ownedNodes;
		for (auto slot : slotMap) {
			// Set attribute-ids
			attributeIds[ids[idIndex]] = slot.first;
			idIndex++;

			// Managed owned nodes
			if (slot.second.ownedByNode) {
				NodeDisplayObj* ownedObj = nullptr;
				for (NodeDisplayObj* currOwned : unmatchedOwned) {
					if (slot.second.mounted == currOwned->elemNode) {
						ownedObj = currOwned;
						break;
					}
				}
				if (ownedObj != nullptr) { // Already existed before
					unmatchedOwned.erase(ownedObj);
				} else { // New
					ownedObj = new NodeDisplayObj();
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

	NodeDisplayObj* updateNode(TreeManager::ElementNode* node) {
		auto found = objMap.find(node);
		NodeDisplayObj* currNode;
		if (found != objMap.end()) {
			currNode = found->second;
		} else {
			currNode = new NodeDisplayObj(); 
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

	void setLinkedToOutput(NodeDisplayObj* nodeObj) {
		linkedToOutput = nodeObj;
		if (nodeObj != nullptr) {
			outputNode->setSelected(nodeObj->elemNode);
		} else {
			outputNode->setSelected(nullptr);
		}
	}

	void resolveUnmountedChildren(TreeManager::ElementNode* node, std::queue<TreeManager::ElementNode*>* queueToFill) {
		for (auto& slot : *node->getSlots()) {
			if (slot.second.mounted == nullptr) continue;

			if (slot.second.ownedByNode) {
				resolveUnmountedChildren(slot.second.mounted, queueToFill);
			} else {
				queueToFill->push(slot.second.mounted);
			}
		}
	}

	void alignNodes(TreeManager::ElementNode* node) {
		struct ProcLevel {
			TreeManager::ElementNode* node;
			std::queue<TreeManager::ElementNode*> childQueue;
			std::vector<std::pair<TreeManager::ElementNode*, ImVec2>> positioned;
			uint64_t totalHeight;
			ElementDisplay* displaySettings;
			ElementDisplay::NodeSize nodeSize;
			uint64_t horPos;
			ProcLevel(TreeManager::ElementNode* node, State* state, size_t horPosOff) 
				: node(node), totalHeight(0), displaySettings(node->getDisplaySettings()), 
					nodeSize(displaySettings->getNodeSize()), horPos(horPosOff + (nodeSize.hasSize ? nodeSize.width : 100)) {
				state->resolveUnmountedChildren(node, &childQueue);
			}
		};
		std::stack<ProcLevel> procStack;
		
		procStack.push(ProcLevel(node, this, 0));

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
					levelCpy.positioned.push_back(std::pair<TreeManager::ElementNode*, ImVec2>(levelCpy.node, {-static_cast<float>(levelCpy.horPos), ownOffset}));
				}
				if (procStack.empty()) {
					// Finish up - Apply positions
					for (auto positioned : levelCpy.positioned) {
						positioned.second.y -= ownOffset;
						auto* dispSettings = positioned.first->getDisplaySettings();
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
				TreeManager::ElementNode* next = currLevel.childQueue.front();
				auto& reposInProgress = next->getDisplaySettings()->reposInProgress;
				if (!reposInProgress) { // Only eval yet unpositioned elements
					procStack.push(ProcLevel(next, this, currLevel.horPos+80));
					reposInProgress = true;
				}
				currLevel.childQueue.pop();
			}
		}
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
		// Create list of existing ones, to check which end up not matched
		std::set<tstudio::NodeDisplayObj*> unmatchedNodeObjs;
		std::transform(objMap.begin(), objMap.end(),
			std::inserter(unmatchedNodeObjs, unmatchedNodeObjs.end()),
			[](auto pair){ return pair.second; });

		// Update/Inser existing ones
		for (auto* node : treeManager->getManagedNodes()) {
			tstudio::NodeDisplayObj* nodeObj = updateNode(node);
			unmatchedNodeObjs.erase(nodeObj); // Remove since this was matched
		}
		// Remove unmatched ones
		for (tstudio::NodeDisplayObj* unmatchedNodeObj : unmatchedNodeObjs) {
			removeNode(unmatchedNodeObj);
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

	struct ContextMenuData {
		bool newElemOpen = false;
		char newElemSearchTerm[1024] = {0x00};
		std::vector<const torasu::ElementFactory*> newElemSearchResult;
		size_t newElemCurrentSelected = 0;
	} contextMenuData;
};

NodeModule::NodeModule() : state(new State()) {}

NodeModule::~NodeModule() {
	delete state;
}

void NodeModule::onMount() {}

namespace {

void renderLinks(const NodeDisplayObj& nodeObj, NodeModule::State* state) {
	auto* slots = nodeObj.elemNode->getSlots(); 

	for (auto attrEntry : nodeObj.attributeIds) {
		NodeDisplayObj::node_id connectedNodeId = -1;
		
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

	for (NodeDisplayObj* owned : nodeObj.ownedNodes) {
		renderLinks(*owned, state);
	}
}

} // namespace

void NodeModule::render(App* instance) {
	bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
	ImNodes::EditorContextSet(state->imnodesEditorContext);
	state->remap(instance);

	if (state->linkedToOutput != nullptr 
		&& state->linkedToOutput->elemNode->getDisplaySettings()->getNodePosition().mode != ElementDisplay::NodePosition::POS_MODE_SET
		&& state->linkedToOutput->elemNode->getDisplaySettings()->hasNodeSize()) {
			state->alignNodes(state->outputNode->getSelected());
	}

	TreeManager::ElementNode* selectNode = nullptr;

	ImNodes::BeginNodeEditor();

	for (auto nodeEntry : state->objMap) {
		TreeManager::ElementNode* node = nodeEntry.first;
		NodeDisplayObj& nodeIds = *nodeEntry.second;

		auto* dispSettings = node->getDisplaySettings();
		ElementDisplay::NodePosition posSettings = dispSettings->getNodePosition();
		if (dispSettings->getVersion() != nodeIds.displayVersion) {
			if (posSettings.mode == ElementDisplay::NodePosition::POS_MODE_UNSET) {
				ImVec2 currPos = ImNodes::GetNodeGridSpacePos(nodeIds.nodeId);
				posSettings = {
					.mode = ElementDisplay::NodePosition::POS_MODE_AUTO,
					.x = currPos.x,
					.y = currPos.y,
				};
				dispSettings->setNodePosition(posSettings);
			}
			ImNodes::SetNodeGridSpacePos(nodeIds.nodeId, {posSettings.x, posSettings.y});
			nodeIds.displayVersion = dispSettings->getVersion();
		}

		ImNodes::BeginNode(nodeIds.nodeId);

		ImNodes::BeginNodeTitleBar();
		const bool originalNodeOpen = !dispSettings->doCollapseNode();
		bool nodeOpen = originalNodeOpen;
		ImGui::Checkbox("", &nodeOpen);
		if (nodeOpen != originalNodeOpen) {
			node->getDisplaySettings()->setCollapseNode(!nodeOpen);
		}
		ImGui::SameLine();
		ImGui::Text("%s #%i", node->getLabel().name, nodeIds.nodeId);
		if (nodeIds.elemNode->isUpdatePending()) {
			ImGui::SameLine();
			Symbols::ApplyingIcon();
		}
		ImNodes::EndNodeTitleBar();

		ElementEditor::renderNodeContents(nodeIds, nodeOpen, &selectNode, state, true);

		ImNodes::EndNode();

		{ // Update position if moved
			ImVec2 currPos = ImNodes::GetNodeGridSpacePos(nodeIds.nodeId);
			if (posSettings.x != currPos.x || posSettings.y != currPos.y) {
				posSettings = {
					.mode = ElementDisplay::NodePosition::POS_MODE_SET,
					.x = currPos.x,
					.y = currPos.y,
				};
				bool upToDate = dispSettings->getVersion() == nodeIds.displayVersion;
				dispSettings->setNodePosition(posSettings);
				if (upToDate) nodeIds.displayVersion = dispSettings->getVersion();
			}
		}

		if (!dispSettings->hasNodeSize()) { // Set Size if not recorded yet
			auto itemSize = ImGui::GetItemRectSize();
			dispSettings->setNodeSize(itemSize.x+20, itemSize.y+20);
		}
	}

	{ // Output Node
		ImNodes::BeginNode(state->outputId);
		ImNodes::BeginNodeTitleBar();
		ImGui::Text("Output #%i", state->outputId);
		if (state->outputNode->hasUpdatePending()) {
			ImGui::SameLine();
			Symbols::ApplyingIcon();
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

	bool editorHovered = ImNodes::IsEditorHovered();
	ImNodes::EndNodeEditor();
	ImVec2 editorPos = ImGui::GetItemRectMin();

	if ((editorHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		|| (focused && (ImGui::GetIO().KeyMods & ImGuiKeyModFlags_Shift) != 0 && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_A), false))) {
		ImGui::OpenPopup("###new-node");;
	}

	if (ImGui::BeginPopup("New Node###new-node")) {
		ImVec2 menuPos = ImGui::GetMousePosOnOpeningCurrentPopup();
		menuPos.x -= editorPos.x;
		menuPos.y -= editorPos.y;
		bool submitSelection = false;
		bool updateSearch = false;
		if (!state->contextMenuData.newElemOpen) {
			ImGui::SetKeyboardFocusHere(0);
			state->contextMenuData.newElemOpen = true;
			updateSearch = true;
		}
		updateSearch |= ImGui::InputText("Search", state->contextMenuData.newElemSearchTerm, sizeof(state->contextMenuData.newElemSearchTerm)/sizeof(char));
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow), true)) {
			state->contextMenuData.newElemCurrentSelected++;
			if (state->contextMenuData.newElemCurrentSelected >= state->contextMenuData.newElemSearchResult.size()) {
				state->contextMenuData.newElemCurrentSelected = 0;
			}
		} else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow), true)) {
			if (state->contextMenuData.newElemCurrentSelected <= 0) {
				state->contextMenuData.newElemCurrentSelected = state->contextMenuData.newElemSearchResult.size();
			}
			state->contextMenuData.newElemCurrentSelected--;
		} else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter), true)) {
			submitSelection = true;
		}

		if (updateSearch) {
			state->contextMenuData.newElemSearchResult = instance->getElementIndex()->getFactoryList(state->contextMenuData.newElemSearchTerm);
			state->contextMenuData.newElemCurrentSelected = 0;
		}

		size_t currItemNum = 0;
		for (const torasu::ElementFactory* matchedFactory : state->contextMenuData.newElemSearchResult) {
			bool selected = currItemNum == state->contextMenuData.newElemCurrentSelected;
			std::string itemLabel = std::string(selected ? ">> " : "") + std::string(matchedFactory->getLabel().name) + "###" + std::string(matchedFactory->getType().str);
			if (!selected) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4({1.0, 1.0, 1.0, 0.6}));
			if (ImGui::MenuItem(itemLabel.c_str()) || (selected && submitSelection)) {
				TreeManager::ElementNode* created = instance->getTreeManager()->addNode(matchedFactory->create(nullptr, torasu::ElementMap()), matchedFactory);
				ImVec2 editorPanning = ImNodes::EditorContextGetPanning();
				created->getDisplaySettings()->setNodePosition({
					.mode = ElementDisplay::NodePosition::POS_MODE_SET,
					.x = menuPos.x-editorPanning.x,
					.y = menuPos.y-editorPanning.y,
				});
				submitSelection = false;
				ImGui::CloseCurrentPopup();
			} else if (ImGui::IsItemHovered()) {
				state->contextMenuData.newElemCurrentSelected = currItemNum;
			}
			if (!selected) ImGui::PopStyleColor();
			currItemNum++;
		}

		ImGui::EndPopup();
	} else {
		state->contextMenuData.newElemOpen = false;
		state->contextMenuData.newElemSearchTerm[0x00] = 0x00;
		state->contextMenuData.newElemSearchResult.clear();
	}

	if (focused && ImGui::IsKeyReleased(ImGui::GetKeyIndex(ImGuiKey_Delete))) {
		size_t numNodes = ImNodes::NumSelectedNodes();
		NodeDisplayObj::node_id ids[numNodes];
		ImNodes::GetSelectedNodes(ids);
		for (NodeDisplayObj::node_id nodeId : ids) {
			auto found = state->idMap.find(nodeId);
			if (found != state->idMap.end()) {
				found->second->elemNode->markForDelete();
			} else {
				throw std::logic_error("Unknown node-id on delete!");
			}
		}	
	}


	ImVec2 editorSize = ImGui::GetItemRectSize();

	NodeDisplayObj::node_id linkReciever;
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
	NodeDisplayObj::node_id linkedNode;
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
	if (selectNode != nullptr) {
		auto foundConnected = state->objMap.find(selectNode);
		if (foundConnected != state->objMap.end()) {
			int selectNodeId = foundConnected->second->nodeId;
			ImNodes::ClearNodeSelection();
			ImNodes::SelectNode(selectNodeId);
			ImVec2 selectedPos = ImNodes::GetNodeGridSpacePos(selectNodeId);
			selectedPos.x *= -1;
			selectedPos.y *= -1;
			selectedPos.x += editorSize.x/2-70;
			selectedPos.y += editorSize.y/2-50;
			ImNodes::EditorContextResetPanning(selectedPos);
		}
		selectNode = nullptr;
	}
}

} // namespace tstudio
