#include "RenderQueue.hpp"

namespace tstudio {

RenderQueue::RenderQueue(torasu::ExecutionInterface* execInterface) 
	: execInterface(execInterface) {}

RenderQueue::~RenderQueue() {}

void RenderQueue::updatePendings() {
	std::vector<uint64_t> retrievedRenderIds;
	for (auto pendingRenderId : pendingRenders) {
		torasu::RenderResult* retrieved = execInterface->tryFetchRenderResult(pendingRenderId);
		if (retrieved != nullptr) {
			resultMap[pendingRenderId] = {.state = ResultState_FINISHED, .result = retrieved};
			retrievedRenderIds.push_back(pendingRenderId);
		}
	}
	for (auto retrievedRenderId : retrievedRenderIds) {
		pendingRenders.erase(retrievedRenderId);
	}
}

bool RenderQueue::requestPause() {
	queueOpen = false;
	return pendingRenders.empty();
}

void RenderQueue::resume() {
	queueOpen = true;
}

bool RenderQueue::mayEnqueue() {
	return queueOpen;
}

uint64_t RenderQueue::enqueueRender(torasu::Renderable* rend, torasu::RenderContext* rctx, torasu::ResultSettings* rs, torasu::LogInstruction li, int64_t prio) {
	auto renderId = execInterface->enqueueRender(rend, rctx, rs, li, prio);
	pendingRenders.insert(renderId);
	return renderId;
}

RenderQueue::ResultState RenderQueue::getResultState(uint64_t renderId) {
	auto found = resultMap.find(renderId);
	if (found != resultMap.end()) {
		return found->second.state;
	} else {
		return ResultState::ResultState_PENDING;
	}
}

torasu::RenderResult* RenderQueue::fetchResult(uint64_t renderId) {
	auto found = resultMap.find(renderId);
	if (found != resultMap.end()) {
		resultMap.erase(found);
		return found->second.result;
	} else {
		throw std::invalid_argument("This render-id was already fetched or never enqueued");
	}
}


} // namespace tstudio
