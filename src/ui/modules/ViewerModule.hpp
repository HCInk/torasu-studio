#ifndef SRC_UI_MODULES_VIEWERMODULE_HPP_
#define SRC_UI_MODULES_VIEWERMODULE_HPP_

#include <stdint.h>

#include "../base/base.hpp"
#include "Module.hpp"

namespace tstudio {

class ViewerModule : public Module {
private:
	void* loadedTexture = nullptr;
	bool texturePending = true;
	bool hasTexture = false;
	void* texture;
	tstudio::TextureId textureId;
public:
	ViewerModule();
	~ViewerModule();
	void onBlank(App* instance, const tstudio::blank_callbacks& callbacks) override;
	void render(App* instance) override;

};

} // namespace tstudio

#endif // SRC_UI_MODULES_VIEWERMODULE_HPP_
