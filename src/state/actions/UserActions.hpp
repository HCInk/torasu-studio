#ifndef SRC_STATE_ACTIONS_USERACTIONS_HPP_
#define SRC_STATE_ACTIONS_USERACTIONS_HPP_

#include <cstddef>
#include <vector>

#include "../App.hpp"

namespace tstudio {

class UserAction;

class UserActions {
private:
	std::vector<UserAction*> undoStack;
	
	void clearUndo();
	void pushUndo(UserAction* action);
	UserAction* popUndo();

public:
	UserActions();
	~UserActions();

	void execute(App* instance, UserAction* action);
	void undo(App* instance);
};

class UserAction {
public:
	void* dependencies = nullptr;
	size_t dependenciesSize = 0;

	enum DependncyUpdateResult {
		/** @brief  Action still available */
		AVAILABLE,
		/** @brief  Action still available, dependencies have updated */
		DEPS_UPDATED,
		/** @brief  Action nolonger available after update */
		UNAVAILABLE
	};

	/**
	 * @brief  Notify action about dependencies about to be removed
	 * @param  instance: Application-instance
	 * @param  removed: pointer array (size removedCount) of dependencies for removal
	 * @param  removedCount: Size of removed array
	 * @retval Result of dependency-update
	 */
	virtual DependncyUpdateResult notifyDependencyRemoval(App* instance, void* removed, size_t removedCount) = 0;

	/**
	 * @brief  Execute Action
	 * @param  instance: Application-instance
	 * @param  generateReverse: Weather it should generate a reverse-action of this action
	 * @retval The reverse action to undo the just executed action (nullptr if not available)
	 */
	virtual UserAction* execute(App* instance, bool generateReverse) = 0;

	virtual ~UserAction() {}
};

} // namespace tstudio


#endif // SRC_STATE_ACTIONS_USERACTIONS_HPP_
