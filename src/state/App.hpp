#ifndef SRC_STATE_APP_HPP_
#define SRC_STATE_APP_HPP_

#include <cstddef>

namespace torasu {
class Renderable;
} // namespace torasu

namespace torasu::tstd {
class EIcore_runner;
} // namespace torasu::tstd

namespace tstudio {
struct blank_callbacks;
class TreeManager;
class Monitor;
class ElementIndex;

class App {
private:
	struct State;
	State* state;

public:
	App();
	~App();

	void onBlank(const tstudio::blank_callbacks& callbacks);

	struct RunnerMetrics {
		size_t queueSize;
		size_t cacheItemCount;
		size_t cacheMemoryUsed;
		size_t cacheMemoryMax;
		bool clearingCache;
	};
	RunnerMetrics getRunnerMetrics();
	void clearRunnerCache();

	TreeManager* getTreeManager();
	Monitor* getMainMonitor();
	ElementIndex* getElementIndex();


	double currentNumber = 0;
};
	
} // namespace tstudio

#endif // SRC_STATE_APP_HPP_
