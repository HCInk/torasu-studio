#ifndef SRC_UI_BASE_ICON_HPP_
#define SRC_UI_BASE_ICON_HPP_

#include <stdint.h>

namespace tstudio {

class RawIcon {
public:
	const uint32_t width;
	const uint32_t height;
	uint8_t*const data;
	RawIcon(uint32_t width, uint32_t height);
	~RawIcon();
};

RawIcon* loadProgramIcon();

} // namespace tstudio


#endif // SRC_UI_BASE_ICON_HPP_
