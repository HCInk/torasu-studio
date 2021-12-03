#ifndef SRC_STATE_EXAMPLES_VIDEOEXAMPLE_CPP_
#define SRC_STATE_EXAMPLES_VIDEOEXAMPLE_CPP_

#include <torasu/std/torasu_full.hpp>
#include <torasu/mod/imgc/imgc_full.hpp>

#include "LoadedExample.hpp"

namespace tstudio::examples {

LoadedExample makeVideoExample() {
	auto* num1 = new torasu::tstd::Rnum(0.5);
	auto* color1 = new imgc::Rcolor(1.0, 1.0, 0.8, num1);
	auto* video = new imgc::Rmedia_file(torasu::tools::inlineRenderable(new torasu::tstd::Rnet_file(
		"https://wasm.hcink.org/torasu/sample-mp4-bunny.mp4"
	)));
	// auto* image = new torasu::tstd::Rmod_rctx(video, torasu::tools::inlineRenderable(new torasu::tstd::Rnum(0)), TORASU_STD_CTX_TIME, TORASU_STD_PL_NUM);
	auto* colorMul = new torasu::tstd::Rmultiply(video, color1);
	auto* text = new imgc::Rtext("TEST");
	auto* textRnd = new imgc::Rgraphics(text);
	auto* roundVal = new torasu::tstd::Rnum(1.0);
	auto* circle = new imgc::Rrothumbus(roundVal);
	auto* circleRnd = new imgc::Rgraphics(circle);
	auto* circleAlign = new imgc::Rauto_align2d(circleRnd, 0.0, 0.0, 0.0, 1.0);
	auto* circleTransform = new imgc::Rtransform(
		torasu::tools::inlineRenderable(circleAlign), 
		torasu::tools::inlineRenderable(new torasu::tstd::Rmatrix({1.0, 0.0, 0.0, 0.0, 1.0, 0.0}, 2)));
	auto* layers = new imgc::Rlayer(torasu::tools::inlineRenderable(
		new torasu::tstd::Rlist({
			circleTransform,
			colorMul,
			// textRnd,
		})
	));
	auto* encode = new imgc::Rmedia_creator(layers, "mp4", 0.0, 10.0, 25.0, 1280, 720, 4000*1000);

	return {
		.managedElements = {video, /* image, */ num1, color1, colorMul, text, textRnd, layers, 
							encode, circleRnd, circle, roundVal, circleTransform},
		.root = encode
	};
}

} // namespace tstudio::examples


#endif // SRC_STATE_EXAMPLES_VIDEOEXAMPLE_CPP_
