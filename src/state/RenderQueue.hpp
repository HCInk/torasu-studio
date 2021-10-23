#ifndef SRC_STATE_RENDERQUEUE_HPP_
#define SRC_STATE_RENDERQUEUE_HPP_

#include <torasu/torasu.hpp>

namespace tstudio {

class RenderQueue {
public:
	enum ResultState {
		ResultState_PENDING = 0,
		ResultState_FINISHED = 1,
		ResultState_CANCELED = 2,
	};
private:
	torasu::ExecutionInterface* execInterface;
	struct ResultStruct {
		ResultState state;
		torasu::RenderResult* result;
	};
	std::set<uint64_t> pendingRenders;
	std::map<uint64_t, ResultStruct> resultMap;

	bool queueOpen = true;

public:
	RenderQueue(torasu::ExecutionInterface* execInterface);
	~RenderQueue();

	/** @brief Fetch pending items enqueued so those will be avaialable in getResultState or fetchResult */
	void updatePendings();
	/**
	 * @brief  Request to pause queue to make modifications
	 * @note   Make sure to call resume() after modifications are done
	 * @retval true: you may now make modifiactions, 
	 * 	         false: pause has been initiated but is not finished yet, please try again later
	 */
	bool requestPause();
	/**
	 * @brief  Resume after pause
	 */
	void resume();
	/**
	 * @brief  Check if starting new renders is allowed
	 * @retval true: yes you may call enqueueRender, false: please try again later
	 */
	bool mayEnqueue();
	uint64_t enqueueRender(torasu::Renderable* rend, torasu::RenderContext* rctx, torasu::ResultSettings* rs, torasu::LogInstruction li, int64_t prio = 0);
	ResultState getResultState(uint64_t renderId);
	torasu::RenderResult* fetchResult(uint64_t renderId);
};

} // namespace tstudio


#endif // SRC_STATE_RENDERQUEUE_HPP_
