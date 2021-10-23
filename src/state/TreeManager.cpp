#include "TreeManager.hpp"

#include <iostream>

namespace tstudio {

TreeManager::TreeManager(const std::map<std::string, const torasu::ElementFactory*>& factories, std::vector<torasu::Element*> elements) 
	: factories(factories) {
	for (auto* element : elements) {
		addNode(element);
	}
}

TreeManager::~TreeManager() {
	for (auto entry : managedElements) {
		delete entry.second;
		delete entry.first;
	}
}

void TreeManager::addNode(torasu::Element* element, const torasu::ElementFactory* factory) {
	if (factory == nullptr) factory = getFactoryForElement(element);
	managedElements[element] = new TreeManager::ElementNode(this, element, factory);
}

void TreeManager::applyUpdates() {
	for (auto* toUpdate : pendingUpdates) {
		toUpdate->applyUpdates();
	}
	pendingUpdates.clear();
}

std::vector<TreeManager::ElementNode*> TreeManager::getManagedNodes() {
	std::vector<ElementNode*> elementList;
	for (auto managedElement : managedElements) {
		elementList.push_back(managedElement.second);
	}
	return elementList;
}

const torasu::ElementFactory* TreeManager::getFactoryForElement(/* const */ torasu::Element* element) {
	auto found = factories.find(element->getType().str);
	return found != factories.end() ? found->second : nullptr;
}

TreeManager::ElementNode* TreeManager::getStoredInstance(const torasu::Element* element) {
	auto found = managedElements.find(element);
	return found != managedElements.end() ? found->second : nullptr;
}

void TreeManager::notifyForUpdate(ElementNode* node) {
	pendingUpdates.push_back(node);
}

TreeManager::ElementNode::ElementNode(TreeManager* manager, torasu::Element* element, const torasu::ElementFactory* elementFactory) 
	: manager(manager), element(element), elementFactory(elementFactory) {
	// Loading slot-index from factory
	if (elementFactory != nullptr) {
		auto slotIndex = elementFactory->getSlotIndex();
		for (size_t i = 0; i < slotIndex.slotCount; i++) {
			auto* currDescriptor = slotIndex.slotIndex+i;
			auto& slot = slots[currDescriptor->id.str];
			slot.descriptor = currDescriptor;
		}
	}
	// Setting contained slots
	for (auto elementSlot : element->getElements()) {
		auto& slot = slots[elementSlot.first];
		torasu::Element* elemInSlot = elementSlot.second;
		ElementNode* foundStored = manager->getStoredInstance(elemInSlot);
		if (foundStored != nullptr) {
			slot.ownedByNode = false;
			slot.mounted = foundStored;
		} else {
			slot.ownedByNode = true;
			slot.mounted = new ElementNode(manager, elemInSlot, manager->getFactoryForElement(elemInSlot));
		}
	}
}

void TreeManager::ElementNode::putSlot(const char* key, TreeManager::ElementNode* node) {
	auto& slot = slots[key];
	if (slot.ownedByNode) delete slot.mounted;
	slot.ownedByNode = false;
	slot.mounted = node;

	updatedSlots.insert(key);
	notifyUpdate();
}

const std::map<std::string, TreeManager::ElementNode::Slot>* TreeManager::ElementNode::getSlots() {
	return &slots;
}

/* const */ torasu::DataResource* TreeManager::ElementNode::getCurrentData() {
	return modifiedData != nullptr ? modifiedData : element->getData();
}

torasu::DataResource* TreeManager::ElementNode::getDataForModification() {
	if (modifiedData == nullptr) {
		const torasu::DataResource* currentData = element->getData();
		modifiedData = currentData->clone();
	}
	notifyUpdate();
	return modifiedData;
}

void TreeManager::ElementNode::notifyUpdate() {
	if (!updatePending) {
		manager->notifyForUpdate(this);
		updatePending = true;
	}
}

void TreeManager::ElementNode::applyUpdates() {
	// Apply Data Modifications
	if (modifiedData != nullptr) {
		try {
			element->setData(modifiedData);
		} catch (const std::exception& ex) {
			std::cerr << "Update data of a " << element->getType().str
				<< " to an instance of " 
				<< (modifiedData != nullptr ? modifiedData->getType().str : "(NONE)")
				<< " failed: " << ex.what() << std::endl;
		}
		modifiedData = nullptr;
	}

	// Apply Slot Modififactions
	for (auto updatedSlot : updatedSlots) {
		auto foundNewSlot = slots.find(updatedSlot);
		ElementNode* newNode = foundNewSlot != slots.end() ? foundNewSlot->second.mounted : nullptr;
		torasu::Element* newElement = newNode != nullptr ? newNode->element : nullptr;
		try {
			element->setElement(updatedSlot, newElement);
		} catch (const std::exception& ex) {
			std::cerr << "Update of slot \"" << updatedSlot << "\" in a " << element->getType().str
				<< " to an instance of " 
				<< (newElement != nullptr ? newElement->getType().str : "(NONE)")
				<< " failed: " << ex.what() << std::endl;
		}
	}
	updatedSlots.clear();

	// Reset pending-state
	updatePending = false;
}

torasu::Identifier TreeManager::ElementNode::getType() {
	return element->getType();
}

torasu::UserLabel TreeManager::ElementNode::getLabel() {
	if (elementFactory != nullptr) {
		return elementFactory->getLabel();
	} else {
		torasu::UserLabel label;
		label.name = element->getType().str;
		return label;
	}
}

TreeManager::ElementNode::~ElementNode() {
	for (auto slot : slots) {
		if (slot.second.ownedByNode) delete slot.second.mounted;
	}
}



} // namespace tstudio
