#ifndef SRC_STATE_MONITORS_IMAGEMONITOR_HPP_
#define SRC_STATE_MONITORS_IMAGEMONITOR_HPP_

#include <torasu/render_tools.hpp>

#include <torasu/std/pipeline_names.hpp>
#include <torasu/std/Dbimg.hpp>

#include "Monitor.hpp"

namespace tstudio {

class ImageMonitor : public Monitor::MonitorImplementation {
private:
	bool update = true;
	bool enqueued = false;
	uint64_t renderId;


	bool updatedSizeSelection = true;
	uint32_t selectedWidth = 1920;
	uint32_t selectedHeight = 1080;
	torasu::tstd::Dbimg* currentImage = nullptr;
	torasu::tstd::Dbimg_FORMAT imgFmt = torasu::tstd::Dbimg_FORMAT(selectedWidth, selectedHeight);
	torasu::tools::ResultSettingsSingleFmt imgSettings = torasu::tools::ResultSettingsSingleFmt(TORASU_STD_PL_VIS, &imgFmt);
public:
	~ImageMonitor();
	torasu::Identifier getMonitorType() const override;
	void resetResults() override;
	void enqueueItems(RenderQueue* renderQueue, torasu::Renderable* renderable, torasu::LogInstruction li, torasu::RenderContext* rctx) override;
	void fetchItems(RenderQueue* renderQueue, std::string* errorText) override;
	void cancelItems(RenderQueue* renderQueue) override;

	inline torasu::tstd::Dbimg* getCurrentImage() { return currentImage; }
	inline uint32_t getSelectedWidth() { return selectedWidth; }
	inline uint32_t getSelectedHeight() { return selectedHeight; }
	inline void setSize(uint32_t selectedWidth, uint32_t selectedHeight) { 
		this->selectedWidth = selectedWidth;
		this->selectedHeight = selectedHeight;
		updatedSizeSelection = true;	
	}
};

} // namespace tstduio


#endif // SRC_STATE_MONITORS_IMAGEMONITOR_HPP_
