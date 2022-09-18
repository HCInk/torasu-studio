#ifndef SRC_UI_MODULES_ACTIONHISTORYMODULE_HPP_
#define SRC_UI_MODULES_ACTIONHISTORYMODULE_HPP_
#include "Module.hpp"

#include <vector>

namespace tstudio {

class ActionHistoryModule : public Module {
public:
	ActionHistoryModule();
	~ActionHistoryModule();
	void render(App* instance) override;

};

} // namespace tstudio

#endif // SRC_UI_MODULES_ACTIONHISTORYMODULE_HPP_
