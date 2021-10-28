#ifndef SRC_UI_MODULES_MODULE_HPP_
#define SRC_UI_MODULES_MODULE_HPP_

#include "../../state/App.hpp"
#include "../base/base.hpp"

namespace tstudio {

class App;

class Module {
public:
	virtual void onMount() {};
	virtual void onBlank(App* instance, const tstudio::blank_callbacks& callbacks) {};
	virtual void render(App* instance) = 0;
	virtual ~Module() {}
};

} // namespace tstudio


#endif // SRC_UI_MODULES_MODULE_HPP_
