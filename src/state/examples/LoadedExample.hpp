#ifndef SRC_STATE_EXAMPLES_LOADEDEXAMPLE_HPP_
#define SRC_STATE_EXAMPLES_LOADEDEXAMPLE_HPP_

#include <vector>
#include <torasu/torasu.hpp>

namespace tstudio::examples {
	
struct LoadedExample {
	std::vector<torasu::Element*> managedElements;
	torasu::Renderable* root;
};

} // namespace tstudio::examples

#endif // SRC_STATE_EXAMPLES_LOADEDEXAMPLE_HPP_
