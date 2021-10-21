#ifndef SRC_UI_MODULES_MODULE_HPP_
#define SRC_UI_MODULES_MODULE_HPP_

namespace tstudio {

class App;

class Module {
public:
	virtual void onMount() {};
	virtual void render(App* instance) = 0;
	virtual ~Module() {}
};

} // namespace tstudio


#endif // SRC_UI_MODULES_MODULE_HPP_
