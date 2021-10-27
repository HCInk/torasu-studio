#ifndef SRC_STATE_MONITORS_NUMBERMONITOR_HPP_
#define SRC_STATE_MONITORS_NUMBERMONITOR_HPP_

#include <torasu/std/Dnum.hpp>

#include "Monitor.hpp"

namespace tstudio {

class NumberMonitor : public Monitor::MonitorImplementation {
private:
	bool update = true;
	bool enqueued = false;
	uint64_t renderId;
	torasu::tstd::Dnum currentNumber;
public:
	torasu::Identifier getMonitorType() const override;
	void resetResults() override;
	void enqueueItems(RenderQueue* renderQueue, torasu::Renderable* renderable, torasu::LogInstruction li, torasu::RenderContext* rctx) override;
	void fetchItems(RenderQueue* renderQueue, std::string* errorText) override;
	void cancelItems(RenderQueue* renderQueue) override;

	inline torasu::tstd::Dnum getCurrentNumber() { return currentNumber; }
};

} // namespace tstudio


#endif // SRC_STATE_MONITORS_NUMBERMONITOR_HPP_
