#include "ElementIndex.hpp"

#include <string>
#include <locale>

namespace tstudio {

namespace {

/**
 * @brief  Change string to upper-case in-place
 * @param  string: nullterminated string array (will be modified)
 */
inline static void toUpperCase(char* string) {
	for (;*string != 0x00; string++) {
		*string = std::toupper(*string);
	}
}

/**
 * @brief  Finds a character-sequence in string sequence
 * @param  toFindIn: String to be searched in (any case, will be converted)
 * @param  toMatch: String to find in toFindIn (has to be upper-case)
 * @retval index where match starts; -1 if no match
 */
inline static int32_t matchStringToUpperCase(const char* toFindIn, const char* toMatch) {
	const char* matchStrLeft = toMatch;
	int32_t beginPos = 0;
	for (int32_t pos = 0; toFindIn[pos] != 0x00; pos++) {
		if (*matchStrLeft == 0x00) {
			// Match once wohle string could be found
			return beginPos;
		}

		if (static_cast<char>(std::toupper(toFindIn[pos])) == *matchStrLeft) {
			// Char match: Check next letter
			matchStrLeft++;
		} else {
			// Char mismatch: Go back in string
			beginPos = pos+1;
			matchStrLeft = toMatch;
		}
	}
	return -1;
} 

} // namespace


ElementIndex::ElementIndex() {}

ElementIndex::~ElementIndex() {}

void ElementIndex::addElementFactory(const torasu::ElementFactory* factory) {
	factoriesByTypeKey[factory->getType().str] = factory;
}


size_t ElementIndex::factoryCount() const {
	return factoriesByTypeKey.size();
}

const torasu::ElementFactory* ElementIndex::getFactoryForElement(/* const */ torasu::Element* element) const {
	auto found = factoriesByTypeKey.find(element->getType().str);
	return found != factoriesByTypeKey.end() ? found->second : nullptr;
}

std::vector<const torasu::ElementFactory*> ElementIndex::getFactoryList(std::string search) const {
	std::vector<const torasu::ElementFactory*> result;
	if (!search.empty()) {
		toUpperCase(search.data());
		for (auto factoryEntry : factoriesByTypeKey) {
			auto factory = factoryEntry.second;
			if (matchStringToUpperCase(factory->getType().str, search.c_str()) >= 0
				|| matchStringToUpperCase(factory->getLabel().name, search.c_str()) >= 0) {
				result.push_back(factory);
			}
		}
	} else {
		for (auto factoryEntry : factoriesByTypeKey) {
			result.push_back(factoryEntry.second);
		}
	}
	return result;
}

} // namespace tstudio
