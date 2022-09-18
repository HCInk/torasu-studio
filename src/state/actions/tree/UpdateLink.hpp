#ifndef SRC_STATE_ACTIONS_TREE_UPDATELINK_HPP_
#define SRC_STATE_ACTIONS_TREE_UPDATELINK_HPP_

#include <set>

#include <torasu/torasu.hpp>
#include "../UserActions.hpp"
#include "../../TreeManager.hpp"

namespace tstudio {

class UpdateLink : public UserAction {
private:
	TreeManager::ElementNode* dst;
	TreeManager::ElementNode* src;
	std::string key;
public:
	UpdateLink(TreeManager::ElementNode* dst, std::string key, TreeManager::ElementNode* src);
	~UpdateLink();

	UserAction::DependncyUpdateResult notifyDependencyRemoval(App* instance, void* removed) override;
	UserAction* execute(App* instance, bool generateReverse) override;
};

} // namespace tstudio

#endif // SRC_STATE_ACTIONS_TREE_UPDATELINK_HPP_
