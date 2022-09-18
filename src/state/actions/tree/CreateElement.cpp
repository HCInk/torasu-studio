#include "CreateElement.hpp"

#include "DeleteElement.hpp"

namespace tstudio {

CreateElement::CreateElement(const torasu::ElementFactory* factory, torasu::DataResource* data, ElementDisplay::NodePosition position, ElementDisplay::NodeSize size, bool collapsed)
 : factory(factory), data(data), position(position), size(size), collapsed(false) {}

CreateElement::~CreateElement() {
	if (data != nullptr) delete data;
}

UserAction::DependncyUpdateResult CreateElement::notifyDependencyRemoval(App* instance, void** removed, size_t removedCount) {
	return AVAILABLE;
}

UserAction* CreateElement::execute(App* instance, bool generateReverse) {
	torasu::ElementMap dummyMap;
	torasu::Element* element = factory->create(data != nullptr ? &data : nullptr, dummyMap);
	tstudio::TreeManager::ElementNode* createdNode = instance->getTreeManager()->addNode(element, factory);
	tstudio::ElementDisplay* displaySettings = createdNode->getDisplaySettings();
	displaySettings->setNodePosition(position);
	displaySettings->setCollapseNode(collapsed);
	if (size.hasSize) {
		displaySettings->setNodeSize(size.width, size.height);
	}
	if (generateReverse) {
		return new DeleteElement(createdNode);
	} else {
		return nullptr;
	}
}


} // namespace tstudio
