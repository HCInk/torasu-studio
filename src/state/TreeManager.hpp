#ifndef SRC_STATE_TREEMANAGER_HPP_
#define SRC_STATE_TREEMANAGER_HPP_

#include <map>
#include <set>

#include <torasu/torasu.hpp>

#include "ElementIndex.hpp"

namespace tstudio {

class ElementDisplay; // from #include "ElementDisplay.hpp"

class TreeManager {
public:
	TreeManager(const ElementIndex* index, std::vector<torasu::Element*> elements = {}, torasu::Element* root = nullptr);
	~TreeManager();

	class ElementNode;
	class OutputNode;

	TreeManager::ElementNode* addNode(torasu::Element* element, const torasu::ElementFactory* factory = nullptr, bool lateInit = false);
	void findUsages(std::vector<std::pair<TreeManager::ElementNode*, std::string>>* found, TreeManager::ElementNode* node);
	bool hasUpdates();
	void applyUpdates();
	std::vector<ElementNode*> getManagedNodes();
	OutputNode* getOutputNode();
	typedef size_t version_t;
	version_t getVersion();

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
		bool markedForDelete = false;
		std::set<std::string> updatedSlots;
		torasu::DataResource* modifiedData = nullptr;

		ElementDisplay* displaySettings;
		
		void notifyUpdate();

	protected:
		ElementNode(TreeManager* manager, torasu::Element* element, const torasu::ElementFactory* elementFactory);
		~ElementNode();

		void applyUpdates();
		void updateLinks();

	public:
		const std::map<std::string, Slot>* getSlots();
		/* const */ torasu::DataResource* getCurrentData();
		void putSlot(const char* key, ElementNode* node);
		torasu::DataResource* getDataForModification();
		void setModifiedData(torasu::DataResource* data);
		void markForDelete();
		bool isMarkedForDelete();

		torasu::Identifier getType();
		torasu::UserLabel getLabel();
		bool isUpdatePending();

		inline ElementDisplay* getDisplaySettings() { return displaySettings; }

		/** Find nodes, which have the given node linked
		 * @param[out] found: Vector to be filled with nodes which have the given node linked 
		 * 	- if the same node is matched multiple times, the all matches are adjacent in this vector 
		 * @param[in] toMatch: Node to be matched (may not be mounted/owned) */
		void findUsages(std::vector<std::pair<TreeManager::ElementNode*, std::string>>* found, TreeManager::ElementNode* toMatch);

		friend TreeManager;
	};
	friend ElementNode;

	class OutputNode {
	private:
		torasu::Renderable* rootRenderable = nullptr;
		ElementNode* selectedRoot = nullptr;
		bool rootUpdatePending = false;
	public:
		torasu::Renderable* getEffective();
		ElementNode* getSelected();
		void setSelected(ElementNode* newRoot);
		inline bool hasUpdatePending() { return rootUpdatePending; }
	protected:
		void update();
		friend TreeManager;
	};

private:
	std::vector<ElementNode*> pendingUpdates;
	std::map<const torasu::Element*, ElementNode*> managedElements;
	OutputNode outputNode;

	version_t version = 0;

protected:
	const ElementIndex* elementIndex;
	ElementNode* getStoredInstance(const torasu::Element* element);
	void notifyForUpdate(ElementNode* node);
};
	
} // namespace tstudio

#endif // SRC_STATE_TREEMANAGER_HPP_
