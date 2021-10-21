#include "state/App.hpp"
#include "ui/base/base.hpp"

int main(int, char**) {
	auto* app = new tstudio::App();
	tstudio::run_base(app);
	return 0;
}