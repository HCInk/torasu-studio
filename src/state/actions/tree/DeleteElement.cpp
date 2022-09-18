#include "DeleteElement.hpp"

#include "../../TreeManager.hpp"
#include "CreateElement.hpp"

namespace tstudio {

DeleteElement::DeleteElement(TreeManager::ElementNode* node) : node(node) {}

DeleteElement::~DeleteElement() {}

UserAction::DependncyUpdateResult DeleteElement::notifyDependencyRemoval(App* instance, void* removed) {
	if (removed == reinterpret_cast<const void*>(node)) return UNAVAILABLE;
	return AVAILABLE;
}

UserAction* DeleteElement::execute(App* instance, bool generateReverse) {
	UserAction* reverseAction = nullptr;
	if (generateReverse) {
		const auto* factory = instance->getElementIndex()->getElementFactoyForIdentifier(node->getType());
		auto dispSettings = node->getDisplaySettings();
		if (factory != nullptr) {
			auto* currData = node->getCurrentData();
			reverseAction = new CreateElement(factory, 
				currData != nullptr ? node->getCurrentData()->clone() : nullptr, 
				dispSettings->getNodePosition(), dispSettings->getNodeSize(), dispSettings->doCollapseNode());
		}
	}
	instance->getUserActions()->notifyDependencyRemoval(instance, node);
	node->markForDelete();
	return reverseAction;
}


} // namespace tstudio
