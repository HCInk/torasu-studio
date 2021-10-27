#ifndef SRC_STATE_MONITOR_HPP_
#define SRC_STATE_MONITOR_HPP_

#include <torasu/torasu.hpp>

#include "../RenderQueue.hpp"

namespace tstudio {

class Monitor {
public:
	enum SelectionMode {
		/** @brief Monitor document-root-node */
		MONITOR_ROOT,
		/** @brief Mointor current user-selection */
		MONITOR_SELECTED,
		/** @brief Monitor what has been set defined by user */
		MONITOR_DEFINED
	};

	class MonitorImplementation {
	protected:
		virtual void resetResults() = 0;
		virtual void enqueueItems(RenderQueue* renderQueue, torasu::Renderable* renderable, torasu::LogInstruction li, torasu::RenderContext* rctx) = 0;
		virtual void fetchItems(RenderQueue* renderQueue, std::string* errorText) = 0;
		virtual void cancelItems(RenderQueue* renderQueue) = 0;
	public:
		virtual ~MonitorImplementation() {};
		virtual torasu::Identifier getMonitorType() const = 0;

		friend Monitor;
	};

private:
	MonitorImplementation* monitorImpl;
	torasu::Renderable* renderable = nullptr;
	torasu::LogInterface* logger;
	torasu::RenderContext rctx;

public:
	SelectionMode selectionMode = MONITOR_DEFINED;
	std::string renderError = "";


	Monitor(MonitorImplementation* monitorImpl, torasu::LogInterface* logger);
	~Monitor();

	void setRenderable(torasu::Renderable* renderable);
	inline torasu::Renderable* getRenderable() { return renderable; }
	inline MonitorImplementation* getMonitorImplementation() { return monitorImpl; }

	void enqueueItems(RenderQueue* renderQueue);
	void fetchItems(RenderQueue* renderQueue);
	void notifiyTreeUpdate();
};

} // namespace tstudio


#endif // SRC_STATE_MONITOR_HPP_
