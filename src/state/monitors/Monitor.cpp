#include "Monitor.hpp"

namespace tstudio {

Monitor::Monitor(MonitorImplementation* monitorImpl, torasu::LogInterface* logger) 
	: monitorImpl(monitorImpl), logger(logger) {}

Monitor::~Monitor() {
	delete monitorImpl;
}

void Monitor::setRenderable(torasu::Renderable* renderable) {
	this->renderable = renderable;
}

void Monitor::enqueueItems(RenderQueue* renderQueue) {
	torasu::LogInstruction li(logger, torasu::INFO);
	monitorImpl->enqueueItems(renderQueue, renderable, li, &rctx);
}

void Monitor::fetchItems(RenderQueue* renderQueue) {
	renderError = "";
	monitorImpl->fetchItems(renderQueue, &renderError);
	if (!renderError.empty()) {
		logger->log(torasu::ERROR, "Monitor-Error: " + renderError);
	}
}

void Monitor::notifiyTreeUpdate() {
	monitorImpl->resetResults();
}

} // namespace tstudio
