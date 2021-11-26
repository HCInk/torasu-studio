#include "ElementDisplay.hpp"

namespace tstudio {

ElementDisplay::ElementDisplay(TreeManager::ElementNode* node) 
	: node(node) {}

void ElementDisplay::setCollapseNode(bool collapse) {
	collapseNode = collapse;
	nodeSize.hasSize = false;
	version++;
}

void ElementDisplay::setNodePosition(NodePosition nodePosition) {
	this->nodePosition = nodePosition;
	version++;
}

void ElementDisplay::setNodeSize(uint32_t width, uint32_t height) {
	nodeSize.hasSize = true;
	nodeSize.width = width;
	nodeSize.height = height;
	version++;
}

} // namespace tstudio