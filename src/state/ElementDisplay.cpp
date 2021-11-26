#include "ElementDisplay.hpp"

namespace tstudio {

ElementDisplay::ElementDisplay(TreeManager::ElementNode* node) 
	: node(node) {}

void ElementDisplay::setCollapseNode(bool collapse) {
	collapseNode = collapse;
	nodeSize.hasSize = false;
}

void ElementDisplay::setNodePosition(NodePosition nodePosition) {
	this->nodePosition = nodePosition;
}

void ElementDisplay::setNodeSize(uint32_t width, uint32_t height) {
	nodeSize.hasSize = true;
	nodeSize.width = width;
	nodeSize.height = height;
}

} // namespace tstudio