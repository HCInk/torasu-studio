#include "NumberMonitor.hpp"

#include <memory>

#include <torasu/render_tools.hpp>
#include <torasu/std/pipeline_names.hpp>

namespace {
torasu::ResultSettings numSettings(TORASU_STD_PL_NUM, torasu::tools::NO_FORMAT);
} // namespace 

namespace tstudio {

torasu::Identifier NumberMonitor::getMonitorType() const {
	return "STD::MONITOR_NUM";
}
	
void NumberMonitor::resetResults() {
	update = true;
}

void NumberMonitor::enqueueItems(RenderQueue* renderQueue, torasu::Renderable* renderable, torasu::LogInstruction li, torasu::RenderContext* rctx) {
	if (update && !enqueued && renderQueue->mayEnqueue()) {
		update = false;
		renderId = renderQueue->enqueueRender(renderable, rctx, &numSettings, li);
		enqueued = true;
	}
}

void NumberMonitor::fetchItems(RenderQueue* renderQueue, std::string* errorText) {
	if (enqueued) {
		RenderQueue::ResultState status = renderQueue->getResultState(renderId);
		if (status != RenderQueue::ResultState_PENDING) {
			std::unique_ptr<torasu::RenderResult> result(renderQueue->fetchResult(renderId));
			enqueued = false;
			if (result) {
				auto* numVal = dynamic_cast<torasu::tstd::Dnum*>(result->getResult());
				if (numVal == nullptr) {
					*errorText = "Render did not return expected type!";
					return;
				}
				currentNumber = *numVal;
			} else if (status == RenderQueue::ResultState_CANCELED) {
				update = true;
			} else {
				*errorText = "Result has not been returned!";
				return;
			}
		}
	}
}

void NumberMonitor::cancelItems(RenderQueue* renderQueue) {
	// TODO cancel items
}

} // namespace tstudio
