#ifndef SRC_UI_BASE_BASE_HPP_
#define SRC_UI_BASE_BASE_HPP_

#include <cstddef>
#include <stdint.h>

#include "../../state/App.hpp"

namespace tstudio {
typedef size_t TextureId;
struct render_hooks {
	struct blank_callbacks {
		TextureId (*create_texture)(uint32_t texWidth, uint32_t texHeight, uint8_t* data) = nullptr;
		void (*update_texture)(TextureId id, uint32_t texWidth, uint32_t texHeight, uint8_t* data) = nullptr;
		void (*destory_texture)(TextureId id) = nullptr;
		void* (*tex_id_to_imgui_id)(TextureId id) = nullptr;
	};
	void (*render_frame)(void) = nullptr;
	void (*on_blank)(const blank_callbacks&) = nullptr;
	void (*post_imgui_init)(const blank_callbacks&) = nullptr;
	void (*pre_imgui_destory)(const blank_callbacks&) = nullptr;
	void (*final_destory)(void) = nullptr;
};

/**
 * @brief  Run UI
 * @param  app: Pointer to app-instance (will be managed/freed by this function)
 */
void run_base(App* app);

} // namespace tstudio


#endif // SRC_UI_BASE_BASE_HPP_
