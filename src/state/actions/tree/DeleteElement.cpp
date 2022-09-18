#include "DeleteElement.hpp"

#include "../../TreeManager.hpp"
#include "../ActionBatch.hpp"
#include "CreateElement.hpp"
#include "UpdateLink.hpp"

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
		// Restore links
		// TODO enable once updating links of recreated elements works
		/* 
		if (reverseAction != nullptr) {
			std::vector<UserAction*> restoreActions;
			for (auto& slot : *node->getSlots()) {
				if (!slot.second.ownedByNode) {
					// TODO detect and ignore defaults once possible
					restoreActions.push_back(new UpdateLink(node, slot.first, slot.second.mounted));
				}
				// TODO handle inlined elements once UpdateLink supports it
			}

			if (!restoreActions.empty()) {
				restoreActions.push_back(reverseAction);
				reverseAction = new ActionBatch(restoreActions, true);
			}
		}
		*/
	}
	instance->getUserActions()->notifyDependencyRemoval(instance, node);
	node->markForDelete();
	return reverseAction;
}


} // namespace tstudio
