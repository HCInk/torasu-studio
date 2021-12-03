#ifndef SRC_UI_MODULES_MONITOR_MONITORMODULE_HPP_
#define SRC_UI_MODULES_MONITOR_MONITORMODULE_HPP_

#include <stdint.h>

#include "../../../state/monitors/Monitor.hpp"
#include "../../base/base.hpp"
#include "../Module.hpp"

namespace tstudio {

class MonitorModule : public Module {
public:
	class MonitorDisplay {
	public:
		virtual bool match(tstudio::Monitor::MonitorImplementation* impl) = 0;
		virtual void onBlank(const tstudio::blank_callbacks& callbacks) {};
		virtual void render() = 0;
		virtual void uninit(const tstudio::blank_callbacks& callbacks) {};
		virtual ~MonitorDisplay() {}
	};
private:
	MonitorDisplay* currentMonitorDisplay = nullptr;

public:
	MonitorModule();
	~MonitorModule();
	void onBlank(App* instance, const tstudio::blank_callbacks& callbacks) override;
	void render(App* instance) override;

};

} // namespace tstudio

#endif // SRC_UI_MODULES_MONITOR_MONITORMODULE_HPP_
