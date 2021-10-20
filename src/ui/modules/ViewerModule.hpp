#ifndef SRC_UI_MODULES_VIEWERMODULE_HPP_
#define SRC_UI_MODULES_VIEWERMODULE_HPP_

#include <stdint.h>

#include "Module.hpp"

namespace tstudio {

class ViewerModule : public Module {
public:
	struct ViewerState {
		static const uint32_t baseTexWidth = 1920;
		static const uint32_t baseTexHeight = 1080;
		float sizeFactor = 1;
		uint32_t texWidth = baseTexWidth*sizeFactor;
		uint32_t texHeight = baseTexHeight*sizeFactor;
		void* image_texture;
		bool reloadTexture = false;
	};
private:
	ViewerState* stateRef;
public:
	ViewerModule(ViewerState* stateRef);
	~ViewerModule();
	void render() override;

};

} // namespace tstudio

#endif // SRC_UI_MODULES_VIEWERMODULE_HPP_
