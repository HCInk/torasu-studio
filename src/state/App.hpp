#ifndef SRC_STATE_APP_HPP_
#define SRC_STATE_APP_HPP_

namespace torasu {
class Renderable;
} // namespace torasu


namespace tstudio {
struct blank_callbacks;
class TreeManager;

class App {
private:
	struct State;
	State* state;

public:
	App();
	~App();

	void onBlank(const tstudio::blank_callbacks& callbacks);

	TreeManager* getTreeManager();
	torasu::Renderable* getRootElement();

	double currentNumber = 0;
};
	
} // namespace tstudio

#endif // SRC_STATE_APP_HPP_
