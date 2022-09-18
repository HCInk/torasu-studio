#ifndef SRC_STATE_ACTIONS_USERACTIONS_HPP_
#define SRC_STATE_ACTIONS_USERACTIONS_HPP_

#include <cstddef>
#include <vector>
#include <string>

#include "../App.hpp"

namespace tstudio {

class UserAction;

class UserActions {
public:
	struct HistoryEntry {
		/** @brief action to be executed, in order to undo action 
		 * @note needs to be freed manually */
		UserAction* action;
		/** @brief label of action */
		std::string label;
	};
private:
	/** @brief undo-stack, oldest at lowest, newest at highest */
	std::vector<HistoryEntry> undoStack;
	/** @brief redo-stack, oldest at lowest, newest at highest */
	std::vector<HistoryEntry> redoStack;
	
	/** @brief Clears all undo-entries */
	void clearUndo();
	/** @brief Clears undo-entries before and at the given index */
	void clearUndoUntil(int64_t index);
	/** @brief Pushes undo-action on the undoStack */
	void pushUndo(HistoryEntry action);
	/** @brief Pops undo-action off the undoStack */
	HistoryEntry popUndo();


	/** @brief Clears all redo-entries */
	void clearRedo();
	/** @brief Clears redo-entries before and at the given index */
	void clearRedoUntil(int64_t index);
	/** @brief Pushes redo-action on the redoStack */
	void pushRedo(HistoryEntry action);
	/** @brief Pops redo-action off the redoStack */
	HistoryEntry popRedo();

public:
	UserActions();
	~UserActions();

	void execute(App* instance, UserAction* action, std::string label);
	void undo(App* instance);
	void redo(App* instance);
	void clearHistory();
	void notifyDependencyRemoval(App* instance, void* removed);

	inline const std::vector<HistoryEntry>& getUndoStack() const { return undoStack; } 
	inline const std::vector<HistoryEntry>& getRedoStack() const { return redoStack; } 
};

class UserAction {
public:
	enum DependncyUpdateResult {
		/** @brief  Action still available */
		AVAILABLE,
		/** @brief  Action still available, dependencies have updated */
		DEPS_UPDATED,
		/** @brief  Action nolonger available after update */
		UNAVAILABLE
	};

	/**
	 * @brief  Notify action about a dependency being about to be removed
	 * @param  instance: Application-instance
	 * @param  removed: Removed dependecy
	 * @retval Result of dependency-update
	 */
	virtual DependncyUpdateResult notifyDependencyRemoval(App* instance, void* removed) = 0;

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
