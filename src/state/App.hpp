#ifndef SRC_STATE_APP_HPP_
#define SRC_STATE_APP_HPP_

namespace torasu {
class Element;
} // namespace torasu


namespace tstudio {

class TreeManager;

class App {
private:
	struct State;
	State* state;
public:
	App();
	~App();

	TreeManager* getTreeManager();
	torasu::Element* getRootElement();
};
	
} // namespace tstudio

#endif // SRC_STATE_APP_HPP_
