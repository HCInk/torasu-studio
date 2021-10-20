#ifndef SRC_UI_MODULES_NODEMODULE_HPP_
#define SRC_UI_MODULES_NODEMODULE_HPP_
#include "Module.hpp"

#include <vector>

namespace tstudio {

class NodeModule : public Module {
private:
	std::vector<std::pair<int, int>> links;
	float someFloat = 0;
	int selectNode = -1;
	bool nodeOpen = true;

	bool isLinked(int attrId, int* otherLink); 
public:
	NodeModule();
	~NodeModule();
	void render() override;
	void onMount() override;

};

} // namespace tstudio

#endif // SRC_UI_MODULES_NODEMODULE_HPP_
