#include "UpdateLink.hpp"

#include "../../TreeManager.hpp"

namespace tstudio {

UpdateLink::UpdateLink(TreeManager::ElementNode* dst, std::string key, TreeManager::ElementNode* src) 
	: dst(dst), key(key), src(src) {}

UpdateLink::~UpdateLink() {}

UserAction::DependncyUpdateResult UpdateLink::notifyDependencyRemoval(App* instance, void* removed) {
	if (removed == reinterpret_cast<const void*>(dst)) return UNAVAILABLE;
	if (src != nullptr && removed == reinterpret_cast<const void*>(src)) return UNAVAILABLE;
	return AVAILABLE;
}

UserAction* UpdateLink::execute(App* instance, bool generateReverse) {
	UserAction* reverseAction = nullptr;
	if (generateReverse) {
		auto* slotMap = dst->getSlots();
		auto foundSlot = slotMap->find(key);
		bool isInlined = false;
		TreeManager::ElementNode* currElem;
		if (foundSlot != slotMap->end()) {
			auto& slot = foundSlot->second;
			isInlined = slot.ownedByNode;
			currElem = slot.mounted;
		} else {
			currElem = nullptr;
		}
		if (isInlined) {
			// TODO generate reversion for inlined elements
			reverseAction = new UpdateLink(dst, key, nullptr);
		} else {
			reverseAction = new UpdateLink(dst, key, currElem);
		}
	}
	dst->putSlot(key.c_str(), src);
	return reverseAction;
}


} // namespace tstudio
