#ifndef SRC_STATE_ELEMENTINDEX_HPP_
#define SRC_STATE_ELEMENTINDEX_HPP_

#include <string>
#include <map>
#include <vector>

#include <torasu/torasu.hpp>

namespace tstudio {

class ElementIndex {
private:
	std::map<std::string, const torasu::ElementFactory*> factoriesByTypeKey;

public:
	ElementIndex();
	~ElementIndex();

	void addElementFactory(const torasu::ElementFactory* factory);

	const torasu::ElementFactory* getElementFactoyForIdentifier(torasu::Identifier ident) const;
	size_t factoryCount() const;
	std::vector<const torasu::ElementFactory*> getFactoryList(std::string search = "") const;

	inline const torasu::ElementFactory* getFactoryForElement(/* const */ torasu::Element* element) const {
		return getElementFactoyForIdentifier(element->getType());
	}
};

} // namespace tstudio

#endif // SRC_STATE_ELEMENTINDEX_HPP_
