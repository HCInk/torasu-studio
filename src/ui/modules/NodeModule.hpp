#ifndef SRC_UI_MODULES_NODEMODULE_HPP_
#define SRC_UI_MODULES_NODEMODULE_HPP_
#include "Module.hpp"

#include <vector>

namespace tstudio {

class NodeModule : public Module {
public:
	struct State;
private:
	State* state;
	std::vector<std::pair<int, int>> links;
	float someFloat = 0;

public:
	NodeModule();
	~NodeModule();
	void render(App* instance) override;
	void onMount() override;

};

} // namespace tstudio

#endif // SRC_UI_MODULES_NODEMODULE_HPP_
