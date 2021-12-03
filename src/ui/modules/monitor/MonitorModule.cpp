#include "MonitorModule.hpp"

#include "../../../state/App.hpp"
#include "../../../state/monitors/Monitor.hpp"
#include "../../../state/monitors/NumberMonitor.hpp"
#include "../../../state/monitors/ImageMonitor.hpp"

#include "ImageMonitorDisplay.hpp"
#include "NumberMonitorDisplay.hpp"

#include <imgui.h>



namespace tstudio {

namespace {

static inline bool displayChanged(Monitor::MonitorImplementation* selectedMonitor, MonitorModule::MonitorDisplay* currentMonitorDisplay) {
	return selectedMonitor != nullptr 
		? currentMonitorDisplay == nullptr || !currentMonitorDisplay->match(selectedMonitor)
		: currentMonitorDisplay != nullptr;
}

} // namespace

MonitorModule::MonitorModule() {}

MonitorModule::~MonitorModule() {}

void MonitorModule::onBlank(App* instance, const tstudio::blank_callbacks& callbacks) {
	auto* selectedMonitor = instance->getMainMonitor()->getMonitorImplementation();
	if (displayChanged(selectedMonitor, currentMonitorDisplay)) { 
		// Reselect Display
		if (currentMonitorDisplay != nullptr) {
			currentMonitorDisplay->uninit(callbacks);
			delete currentMonitorDisplay;
			currentMonitorDisplay = nullptr;
		}

		do { 
			if ((currentMonitorDisplay = ImageMonitorDisplay::matchAndCreate(selectedMonitor)) != nullptr) break;
			if ((currentMonitorDisplay = NumberMonitorDisplay::matchAndCreate(selectedMonitor)) != nullptr) break;
		} while(false);
	}

	if (currentMonitorDisplay != nullptr) {
		currentMonitorDisplay->onBlank(callbacks);
	}
}

void MonitorModule::render(App* instance) {
	if (displayChanged(instance->getMainMonitor()->getMonitorImplementation(), currentMonitorDisplay)) {
		currentMonitorDisplay = nullptr;
		ImGui::TextUnformatted("Updating Display...");
		return;
	}

	if (currentMonitorDisplay != nullptr) {
		currentMonitorDisplay->render();
	} else {
		ImGui::TextUnformatted("(No Display)");
	}
}

} // namespace tstudio
