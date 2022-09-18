#ifndef SRC_STATE_ACTIONS_ACTIONBATCH_HPP_
#define SRC_STATE_ACTIONS_ACTIONBATCH_HPP_

#include <vector>

#include "UserActions.hpp"

namespace tstudio {

class ActionBatch : public UserAction {
private:
	/** @brief actions to be processed in batch */
	std::vector<UserAction*> actions;
	/** @brief true: a successful rever to this batch will be possible and revert should be available
	 * 		false: revert is not available */
	bool revertable;
public:
	/**
	 * @brief  Create Batch for bundeling multiple operations
	 * @param  actions: List of actions to be run subsequently to form this bundled action
	 * @param  revertable: true: Reversion available, false: No reversion available
	 * @note   This may only be set to revertable if reverting of these actions is always possible 
	 * 	(and reverting of those reversions etc...)
	 */
	ActionBatch(std::vector<UserAction*> actions, bool revertable);
	~ActionBatch();

	UserAction::DependncyUpdateResult notifyDependencyRemoval(App* instance, void* removed) override;
	UserAction* execute(App* instance, bool generateReverse) override;
};

} // namespace tstudio

#endif // SRC_STATE_ACTIONS_ACTIONBATCH_HPP_
