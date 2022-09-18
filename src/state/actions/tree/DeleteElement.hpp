#ifndef SRC_STATE_ACTIONS_TREE_DELETEELEMENT_HPP_
#define SRC_STATE_ACTIONS_TREE_DELETEELEMENT_HPP_

#include <torasu/torasu.hpp>
#include "../UserActions.hpp"
#include "../../TreeManager.hpp"

namespace tstudio {

class DeleteElement : public UserAction {
private:
	TreeManager::ElementNode* node;
public:
	DeleteElement(TreeManager::ElementNode* node);
	~DeleteElement();

	UserAction::DependncyUpdateResult notifyDependencyRemoval(App* instance, void* removed) override;
	UserAction* execute(App* instance, bool generateReverse) override;
};

} // namespace tstudio

#endif // SRC_STATE_ACTIONS_TREE_DELETEELEMENT_HPP_
