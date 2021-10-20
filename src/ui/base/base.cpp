// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#define BACKEND_MODE 0 // 0 = glfw/opengl 1 = glfw/webgpu

#include <vector>
#include <utility>
#include <iostream>
#include <cmath>
#include <memory>
#include <imgui.h>
#include <stdio.h>

#include "glfw_runner.hpp"

// #include "../thirdparty/imgui-node-editor/imgui_node_editor.h"
// namespace nodes = ax::NodeEditor;
#include "../../../thirdparty/imnodes/imnodes.h"

#include "../modules/NodeModule.hpp"
#include "../modules/ViewerModule.hpp"

namespace tstudio {
static void main_loop();

std::string generateIniWindow(std::string title, int32_t xPos, int32_t yPos, uint32_t width, uint32_t height, std::string dockId = "", size_t tabNo = 0) {
	return "[Window][" + title + "]\n"
		"Pos=" + std::to_string(xPos) + "," + std::to_string(yPos) + "\n"
		"Size=" + std::to_string(width) + "," + std::to_string(height) + "\n"
		"Collapsed=0\n"
		+ (!dockId.empty() ? "DockId=" + dockId + "," + std::to_string(tabNo) + "\n" : "")
		+ "\n";
}

std::string generateIniCommands() {
	std::string rootDockId = "0x8B93E3BD";

	std::string iniStr = "";
	
	iniStr += generateIniWindow("###node-viewer", 0, 0, 1920, 1080, "0x00000004", 0);
	iniStr += generateIniWindow("###splash-screen", 0, 0, 1920, 1080, "0x00000002", 0);
	iniStr += generateIniWindow("###result-viewer", 0, 0, 1920, 1080, "0x00000003", 0);
	iniStr += generateIniWindow("Dear ImGui Demo", 0, 0, 1920, 1080, "0x00000003", 1);

	iniStr += "[Docking][Data]\n"
		"DockSpace     ID=" + rootDockId + " Window=0xA787BDB4 Pos=0,0 Size=1920,1080 Split=Y Selected=0x25A319F0\n"
		"  DockNode    ID=0x00000001 Parent=" + rootDockId + " SizeRef=1920,259 Split=X Selected=0xEBE6C6E6\n"
		"    DockNode  ID=0x00000002 Parent=0x00000001 SizeRef=1587,537 Selected=0xEBE6C6E6\n"
		"    DockNode  ID=0x00000003 Parent=0x00000001 SizeRef=331,537 Selected=0x25A319F0\n"
		"  DockNode    ID=0x00000004 Parent=" + rootDockId + " SizeRef=1920,819 Selected=0x2DD179D7\n"
		"\n";

	return iniStr;
}

auto generateTexture(uint32_t texWidth, uint32_t texHeight) {
	size_t texDataSize = texWidth*texHeight*4;

	std::unique_ptr<uint8_t, std::default_delete<uint8_t[]>> texDataHoler(new uint8_t[texDataSize]);

	uint8_t* texData = texDataHoler.get();

	size_t i = 0;
	for (int32_t y = 0; y < texHeight; y++) {

		for (int32_t x = 0; x < texWidth; x++) {
			texData[i] = y;
			i++;
			texData[i] = x;
			i++;
			texData[i] = x*y;
			// texData[i] = (x-y)-0xFF*(std::floor(static_cast<float>(x-y)/0xFF)) > 0x88 ? 0xFF : 0x00;
			i++;
			texData[i] = 0xFF;
			i++;
		}

	}

	return texDataHoler;
}

namespace {
tstudio::TextureId image_texture_id;
static bool reloadLayout = true;
static NodeModule nodeModule = NodeModule();
static ViewerModule::ViewerState viewerState;
static ViewerModule viewerModule = ViewerModule(&viewerState);

static void post_imgui_init(const tstudio::render_hooks::blank_callbacks& callbacks) {
    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

	ImNodes::CreateContext();

	auto generated = generateTexture(viewerState.texWidth, viewerState.texHeight);
	image_texture_id = callbacks.create_texture(viewerState.texWidth, viewerState.texHeight, generated.get());
	viewerState.image_texture = callbacks.tex_id_to_imgui_id(image_texture_id);

	nodeModule.onMount();
	viewerModule.onMount();
}

void on_blank(const tstudio::render_hooks::blank_callbacks& callbacks) {
	if (reloadLayout) {
		std::string iniCommands = generateIniCommands();
		ImGui::LoadIniSettingsFromMemory(iniCommands.c_str());
		reloadLayout = false;
	}
	if (viewerState.reloadTexture) {
		auto generated = generateTexture(viewerState.texWidth, viewerState.texHeight);
		callbacks.update_texture(image_texture_id, viewerState.texWidth, viewerState.texHeight, generated.get());
		viewerState.reloadTexture = false;
	}
}

static void pre_imgui_destory(const tstudio::render_hooks::blank_callbacks& callbacks) {
	callbacks.destory_texture(image_texture_id);
	ImNodes::DestroyContext();
}


} // namespace

void run_base() {
	render_hooks hooks;
	hooks.render_frame = main_loop;
	hooks.on_blank = on_blank;
	hooks.post_imgui_init = post_imgui_init;
	hooks.pre_imgui_destory = pre_imgui_destory;
	glfw_run(hooks);
}

static void main_loop() {
	// Our state
	static bool show_demo_window = true;
	static bool show_another_window = true;
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	ImGui::DockSpaceOverViewport(nullptr, 0, nullptr);

	{
		ImGui::Begin("Node-Editor###node-viewer");

		nodeModule.render();

		ImGui::End();
	}

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;


		
		ImGui::Begin("Welcome!###splash-screen");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);
		ImGui::SameLine();
		if (ImGui::Button("Reset Layout"))
			reloadLayout = true;

		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Viewer###result-viewer", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		viewerModule.render();

		ImGui::End();
	}

	auto draw = ImGui::GetBackgroundDrawList();
	draw->AddRect(ImVec2(0, 0), ImVec2(10, 10), 0, 0, 0, 2);

}

} // namespace tstudio