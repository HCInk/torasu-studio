#include "TreeManager.hpp"
#include "ElementDisplay.hpp"

#include <iostream>

namespace tstudio {

TreeManager::TreeManager(const ElementIndex* elementIndex, std::vector<torasu::Element*> elements, torasu::Element* root) 
	: elementIndex(elementIndex) {
	for (auto* element : elements) {
		addNode(element, nullptr, true);
	}
	for (auto managed : managedElements) {
		managed.second->updateLinks();
	}
	if (root != nullptr) {
		auto foundRoot = managedElements.find(root);
		if (foundRoot != managedElements.end()) {
			outputNode.setSelected(foundRoot->second);
			outputNode.update();
		} else {
			throw std::logic_error("Failed to find root element in list of managed elements!");
		}
	}
}

TreeManager::~TreeManager() {
	for (auto entry : managedElements) {
		delete entry.second;
		delete entry.first;
	}
}

TreeManager::ElementNode* TreeManager::addNode(torasu::Element* element, const torasu::ElementFactory* factory, bool lateInit) {
	if (factory == nullptr) factory = elementIndex->getFactoryForElement(element);
	auto* node = new TreeManager::ElementNode(this, element, factory);
	if (!lateInit) {
		node->updateLinks();
		version++;
	}
	managedElements[element] = node;
	return node;
}

void TreeManager::findUsages(std::vector<std::pair<TreeManager::ElementNode*, std::string>>* found, TreeManager::ElementNode* node) {
	for (auto managedEntry : managedElements) {
		managedEntry.second->findUsages(found, node);
	}
}

bool TreeManager::hasUpdates() {
	return !pendingUpdates.empty() || outputNode.hasUpdatePending();
}

void TreeManager::applyUpdates() {
	for (tstudio::TreeManager::ElementNode* toUpdate : pendingUpdates) {
		if (toUpdate->isMarkedForDelete()) {
			torasu::Element* element = toUpdate->element;
			auto found = managedElements.find(element);
			if (found != managedElements.end()) {
				std::vector<std::pair<ElementNode*, std::string>> usages;
				findUsages(&usages, toUpdate);
				for (auto usage : usages) {
					ElementNode* user = usage.first;
					user->putSlot(usage.second.c_str(), nullptr);
					user->applyUpdates();
				}
				managedElements.erase(found);
				delete toUpdate;
				delete element;
			} else {
				// Only managed can be marked for deletion / for mounted the elements have to be overwritten in the slot
				throw std::runtime_error("Could not find element which was marked for deletion in managed-list.");
			}
		} else {
			toUpdate->applyUpdates();
		}
	}
	pendingUpdates.clear();
	if (outputNode.hasUpdatePending()) {
		outputNode.update();
	}
	version++;
}

std::vector<TreeManager::ElementNode*> TreeManager::getManagedNodes() {
	std::vector<ElementNode*> elementList;
	for (auto managedElement : managedElements) {
		elementList.push_back(managedElement.second);
	}
	return elementList;
}

TreeManager::OutputNode* TreeManager::getOutputNode() {
	return &outputNode;
}

torasu::Renderable* TreeManager::OutputNode::getEffective() {
	return rootRenderable;
}

TreeManager::ElementNode* TreeManager::OutputNode::getSelected() {
	return selectedRoot;
}

void TreeManager::OutputNode::setSelected(ElementNode* newRoot) {
	selectedRoot = newRoot;
	rootUpdatePending = true;
}

void TreeManager::OutputNode::update() {
	if (selectedRoot != nullptr) {
		if (torasu::Renderable* newRoot = dynamic_cast<torasu::Renderable*>(selectedRoot->element)) {
			rootRenderable = newRoot;
		} else {
			rootRenderable = nullptr;
			selectedRoot = nullptr;
		}
	} else {
		rootRenderable = nullptr;
	}
	rootUpdatePending = false;
}

TreeManager::version_t TreeManager::getVersion() {
	return version;
}

TreeManager::ElementNode* TreeManager::getStoredInstance(const torasu::Element* element) {
	auto found = managedElements.find(element);
	return found != managedElements.end() ? found->second : nullptr;
}

void TreeManager::notifyForUpdate(ElementNode* node) {
	pendingUpdates.push_back(node);
}

TreeManager::ElementNode::ElementNode(TreeManager* manager, torasu::Element* element, const torasu::ElementFactory* elementFactory) 
	: manager(manager), element(element), elementFactory(elementFactory), displaySettings(new ElementDisplay(this)) {
	// Loading slot-index from factory
	if (elementFactory != nullptr) {
		auto slotIndex = elementFactory->getSlotIndex();
		for (size_t i = 0; i < slotIndex.slotCount; i++) {
			auto* currDescriptor = slotIndex.slotIndex+i;
			auto& slot = slots[currDescriptor->id.str];
			slot.descriptor = currDescriptor;
		}
	}
}

void TreeManager::ElementNode::updateLinks() {
	// Setting contained slots
	for (auto elementSlot : element->getElements()) {
		auto& slot = slots[elementSlot.first];
		if (slot.ownedByNode) {
			delete slot.mounted;
		}
		torasu::Element* elemInSlot = elementSlot.second;
		if (elemInSlot != nullptr) {

			ElementNode* foundStored = manager->getStoredInstance(elemInSlot);
			if (foundStored != nullptr) {
				slot.ownedByNode = false;
				slot.mounted = foundStored;
			} else {
				slot.ownedByNode = true;
				slot.mounted = new ElementNode(manager, elemInSlot, manager->elementIndex->getFactoryForElement(elemInSlot));
				slot.mounted->updateLinks();
			}
		} else {
			slot.ownedByNode = false;
			slot.mounted = nullptr;
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

void TreeManager::ElementNode::setModifiedData(torasu::DataResource* data) {
	auto* oldModifiedData = modifiedData;
	modifiedData = data;
	if (oldModifiedData != nullptr) delete oldModifiedData;
	notifyUpdate();
}

void TreeManager::ElementNode::markForDelete() {
	markedForDelete = true;
	notifyUpdate();
}

bool TreeManager::ElementNode::isMarkedForDelete() {
	return markedForDelete;
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

bool TreeManager::ElementNode::isUpdatePending() {
	return updatePending;
}

void TreeManager::ElementNode::findUsages(std::vector<std::pair<TreeManager::ElementNode*, std::string>>* found, TreeManager::ElementNode* toMatch) {
	bool added = false;
	for (auto slot : slots) {
		if (slot.second.ownedByNode) {
			// Recurse into mounted
			slot.second.mounted->findUsages(found, toMatch);
		} else if (!added && slot.second.mounted == toMatch) {
			// Collect all which have the node to be matched liked
			found->push_back({this, slot.first});
		}
	}
}

TreeManager::ElementNode::~ElementNode() {
	for (auto slot : slots) {
		if (slot.second.ownedByNode) delete slot.second.mounted;
	}
	delete displaySettings;
}



} // namespace tstudio
