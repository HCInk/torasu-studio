#ifndef SRC_STATE_ACTIONS_TREE_CREATEELEMENT_HPP_
#define SRC_STATE_ACTIONS_TREE_CREATEELEMENT_HPP_

#include <torasu/torasu.hpp>
#include "../UserActions.hpp"
#include "../../ElementDisplay.hpp"

namespace tstudio {

class CreateElement : public UserAction {
private:
	const torasu::ElementFactory* factory;
	torasu::DataResource* data = nullptr;
	ElementDisplay::NodePosition position;
	ElementDisplay::NodeSize size;
	bool collapsed;
public:
	CreateElement(const torasu::ElementFactory* factory, torasu::DataResource* data, ElementDisplay::NodePosition position, ElementDisplay::NodeSize size = {.hasSize=false}, bool collapsed = false);
	~CreateElement();

	UserAction::DependncyUpdateResult notifyDependencyRemoval(App* instance, void* removed, size_t removedCount) override;
	UserAction* execute(App* instance, bool generateReverse) override;
};

} // namespace tstudio

#endif // SRC_STATE_ACTIONS_TREE_CREATEELEMENT_HPP_
