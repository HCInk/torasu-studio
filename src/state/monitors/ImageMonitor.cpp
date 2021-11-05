#include "ImageMonitor.hpp"

#include <memory>

#include <torasu/render_tools.hpp>
#include <torasu/std/pipeline_names.hpp>
#include <torasu/std/context_names.hpp>

namespace tstudio {

ImageMonitor::~ImageMonitor() {
	if (currentImage != nullptr) delete currentImage;
}

torasu::Identifier ImageMonitor::getMonitorType() const {
	return "STD::MONITOR_NUM";
}
	
void ImageMonitor::resetResults() {
	update = true;
}

void ImageMonitor::enqueueItems(RenderQueue* renderQueue, torasu::Renderable* renderable, torasu::LogInstruction li, torasu::RenderContext* rctx) {
	if (updatedSizeSelection && !enqueued) {
		imgFmt = torasu::tstd::Dbimg_FORMAT(selectedWidth, selectedHeight);
		ratioNum = static_cast<double>(selectedWidth)/selectedHeight;
		update = true;
		updatedSizeSelection = false;
	}
	if (update && !enqueued && renderQueue->mayEnqueue()) {
		update = false;
		(*rctx)[TORASU_STD_CTX_IMG_RATIO] = &ratioNum;
		renderId = renderQueue->enqueueRender(renderable, rctx, &imgSettings, li);
		enqueued = true;
	}
}

void ImageMonitor::fetchItems(RenderQueue* renderQueue, std::string* errorText) {
	if (enqueued) {
		RenderQueue::ResultState status = renderQueue->getResultState(renderId);
		if (status != RenderQueue::ResultState_PENDING) {
			std::unique_ptr<torasu::RenderResult> result(renderQueue->fetchResult(renderId));
			enqueued = false;
			if (result->getResult() != nullptr) {
				auto* image = dynamic_cast<torasu::tstd::Dbimg*>(result->getResult());
				if (image == nullptr) {
					*errorText = "Render did not return expected type!";
					return;
				}
				auto* oldImg = currentImage;
				currentImage = static_cast<torasu::tstd::Dbimg*>(result->ejectOrClone());
				if (oldImg != nullptr) delete oldImg;
			} else if (status == RenderQueue::ResultState_CANCELED) {
				update = true;
			} else {
				*errorText = "Result has not been returned!";
				return;
			}
		}
	}
}

void ImageMonitor::cancelItems(RenderQueue* renderQueue) {
	// TODO cancel items
}

} // namespace tstudio
