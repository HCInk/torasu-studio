#ifndef SRC_UI_MODULES_MONITOR_NUMBERMONITORDISPLAY_HPP_
#define SRC_UI_MODULES_MONITOR_NUMBERMONITORDISPLAY_HPP_

#include "../../../state/monitors/NumberMonitor.hpp"

#include "MonitorModule.hpp"

namespace tstudio {

class NumberMonitorDisplay : public MonitorModule::MonitorDisplay {
private:
	NumberMonitor* numMon;

public:
	NumberMonitorDisplay(NumberMonitor* numMon);
	~NumberMonitorDisplay();

	static NumberMonitorDisplay* matchAndCreate(Monitor::MonitorImplementation* impl);
	bool match(Monitor::MonitorImplementation* impl) override;
	void render() override;
};

} // namespace tstudio


#endif // SRC_UI_MODULES_MONITOR_NUMBERMONITORDISPLAY_HPP_
