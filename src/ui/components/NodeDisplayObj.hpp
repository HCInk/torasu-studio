#ifndef SRC_UI_COMPONENTS_NODEDISPLAYOBJ_HPP_
#define SRC_UI_COMPONENTS_NODEDISPLAYOBJ_HPP_

#include "../../state/TreeManager.hpp"
#include "../../state/ElementDisplay.hpp"

namespace tstudio {

/** @brief Struct which contains relevant data required for display for example in the 
 * node-editor or other modules */
struct NodeDisplayObj {
	typedef int node_id;
	TreeManager::ElementNode* elemNode;
	std::set<NodeDisplayObj*> ownedNodes; 
	ElementDisplay::version_t displayVersion = 0;
	// Node-View-Specific
	node_id nodeId;
	std::map<node_id, std::string> attributeIds; 
};

} // namespace tstudio

#endif // SRC_UI_COMPONENTS_NODEDISPLAYOBJ_HPP_
