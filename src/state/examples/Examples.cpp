#ifndef SRC_STATE_EXAMPLES_EXAMPLES_CPP_
#define SRC_STATE_EXAMPLES_EXAMPLES_CPP_

#include "LoadedExample.hpp"

#include "VideoExample.cpp"

namespace tstudio::examples {

LoadedExample(*makeSelectedExample)(void) = makeVideoExample;

} // namespace tstudio::examples

#endif // SRC_STATE_EXAMPLES_EXAMPLES_CPP_
