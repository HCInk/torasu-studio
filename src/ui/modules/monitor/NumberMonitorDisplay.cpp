#include "NumberMonitorDisplay.hpp"

#include <imgui.h>

namespace tstudio {

NumberMonitorDisplay::NumberMonitorDisplay(NumberMonitor* numMon) : numMon(numMon) {}

NumberMonitorDisplay::~NumberMonitorDisplay() {}

NumberMonitorDisplay* NumberMonitorDisplay::matchAndCreate(Monitor::MonitorImplementation* impl) {
	if (auto* casted = dynamic_cast<NumberMonitor*>(impl)) {
		return new NumberMonitorDisplay(casted);
	}
	return nullptr;
}

bool NumberMonitorDisplay::match(Monitor::MonitorImplementation* impl) {
	return numMon == impl && dynamic_cast<NumberMonitor*>(impl) != nullptr;
}

void NumberMonitorDisplay::render() {
	ImGui::Text("Current Number: %f", numMon->getCurrentNumber().getNum());
}

} // namespace tstudio
