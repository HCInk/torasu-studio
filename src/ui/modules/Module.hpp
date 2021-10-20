#ifndef SRC_UI_MODULES_MODULE_HPP_
#define SRC_UI_MODULES_MODULE_HPP_

namespace tstudio {

class Module {
public:
	virtual void onMount() {};
	virtual void render() = 0;
	virtual ~Module() {}
};

} // namespace tstudio


#endif // SRC_UI_MODULES_MODULE_HPP_
