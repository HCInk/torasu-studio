#include "Icon.hpp"

#include <vector>
#include <string>

namespace {

#include "../../../thirdparty/picopng.cpp"

extern "C" {
#include "StoredIcons.c"
}

} // namespace


namespace tstudio {

RawIcon::RawIcon(uint32_t width, uint32_t height) 
	: width(width), height(height), data(new uint8_t[width*height*4]) {}

RawIcon::~RawIcon() {
	delete[] data;
};

RawIcon* loadProgramIcon() {
	std::vector<uint8_t> out_image;
	uint64_t image_width;
	uint64_t image_height;
	decodePNG(out_image, image_width, image_height, TORASU_LOGO_ALPHA_PNG, sizeof(TORASU_LOGO_ALPHA_PNG), true);
	auto* icon = new RawIcon(image_width, image_height);
	uint8_t* src = out_image.data();
	uint8_t* dest = icon->data;
	for (uint64_t y = 0; y < image_height; y++) {
		for (uint64_t x = 0; x < image_width; x++) {
			float pos = static_cast<float>(x+(image_height-y))/image_height/2;
			
			*dest = (1-pos)*0x33+0xCC;
			dest++;
			*dest = (pos)*0x77+0x88;
			dest++;
			*dest = 0x00;
			dest++;

			*dest = *src;
			dest++;
			src+=4;
		}
	}
	return icon;
}

} // namespace tstudio
