#ifndef SRC_UI_COMPONENTS_ELEMENTEDITOR_HPP_
#define SRC_UI_COMPONENTS_ELEMENTEDITOR_HPP_

#include "../../state/TreeManager.hpp"
#include "../modules/NodeModule.hpp"
#include "NodeDisplayObj.hpp"

namespace tstudio {

class ElementEditor {
public:
	static void renderDataEditor(TreeManager::ElementNode* node);
	static void renderNodeContents(const NodeDisplayObj& nodeIds, bool nodeOpen, TreeManager::ElementNode** selectNode, NodeModule::State* state, bool renderOutput);
};

} // namespace tstudio

#endif // SRC_UI_COMPONENTS_ELEMENTEDITOR_HPP_
