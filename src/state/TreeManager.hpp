#ifndef SRC_STATE_TREEMANAGER_HPP_
#define SRC_STATE_TREEMANAGER_HPP_

#include <map>
#include <set>

#include <torasu/torasu.hpp>

namespace tstudio {

class TreeManager {
public:
	TreeManager(const std::map<std::string, const torasu::ElementFactory*>& factories, std::vector<torasu::Element*> elements = {});
	~TreeManager();

	class ElementNode;

	void addNode(torasu::Element* element, const torasu::ElementFactory* factory = nullptr);
	void applyUpdates();
	std::vector<ElementNode*> getManagedNodes();

	class ElementNode {
	private:
		TreeManager* manager;
		torasu::Element* element;
		const torasu::ElementFactory* elementFactory = nullptr;

		struct Slot {
			const torasu::ElementFactory::SlotDescriptor* descriptor = nullptr;
			ElementNode* mounted = nullptr;
			bool ownedByNode = false;
		};

		std::map<std::string, Slot> slots;

		bool updatePending = false;
		std::set<std::string> updatedSlots;
		torasu::DataResource* modifiedData = nullptr;
		
		void notifyUpdate();

	protected:
		ElementNode(TreeManager* manager, torasu::Element* element, const torasu::ElementFactory* elementFactory);
		~ElementNode();

		void applyUpdates();

	public:
		const std::map<std::string, Slot>* getSlots();
		/* const */ torasu::DataResource* getCurrentData();
		void putSlot(const char* key, ElementNode* node);
		torasu::DataResource* getDataForModification();

		torasu::UserLabel getLabel();

		friend TreeManager;
	};
	friend ElementNode;

private:
	std::vector<ElementNode*> pendingUpdates;
	std::map<const torasu::Element*, ElementNode*> managedElements;
	std::map<std::string, const torasu::ElementFactory*> factories;

protected:
	const torasu::ElementFactory* getFactoryForElement(/* const */ torasu::Element* element);
	ElementNode* getStoredInstance(const torasu::Element* element);
	void notifyForUpdate(ElementNode* node);
};
	
} // namespace tstudio

#endif // SRC_STATE_TREEMANAGER_HPP_
