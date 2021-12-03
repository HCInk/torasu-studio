#ifndef SRC_UI_MODULES_MONITOR_IMAGEMONITORDISPLAY_HPP_
#define SRC_UI_MODULES_MONITOR_IMAGEMONITORDISPLAY_HPP_

#include "MonitorModule.hpp"
#include "../../../state/monitors/ImageMonitor.hpp"

namespace tstudio {

class ImageMonitorDisplay : public MonitorModule::MonitorDisplay {
private:
	ImageMonitor* imageMon;
	void* loadedTexture = nullptr;
	bool texturePending = true;
	bool hasTexture = false;
	void* texture;
	tstudio::TextureId textureId;

	void updateTexture(const tstudio::blank_callbacks& callbacks, bool destroyDisplay);

public:
	ImageMonitorDisplay(ImageMonitor* imageMon);
	~ImageMonitorDisplay();

	static ImageMonitorDisplay* matchAndCreate(Monitor::MonitorImplementation* impl);
	bool match(Monitor::MonitorImplementation* impl) override;
	void onBlank(const tstudio::blank_callbacks& callbacks) override;
	void uninit(const tstudio::blank_callbacks& callbacks) override;
	void render() override;
};

} // namespace tstudio


#endif // SRC_UI_MODULES_MONITOR_IMAGEMONITORDISPLAY_HPP_
