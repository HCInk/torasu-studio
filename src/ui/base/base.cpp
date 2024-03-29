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

#include <torasu/std/EIcore_runner.hpp>

#include "glfw_runner.hpp"

// #include "../thirdparty/imgui-node-editor/imgui_node_editor.h"
// namespace nodes = ax::NodeEditor;
#include "../../../thirdparty/imnodes/imnodes.h"

#include "../modules/NodeModule.hpp"
#include "../modules/ActionHistoryModule.hpp"
#include "../modules/monitor/MonitorModule.hpp"

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
	
	iniStr += generateIniWindow("###node-viewer", 0, 0, 1920, 1080, "0x00000005", 0);
	iniStr += generateIniWindow("###node-viewer2", 0, 0, 1920, 1080, "0x00000006", 0);
	iniStr += generateIniWindow("###splash-screen", 0, 0, 1920, 1080, "0x00000007", 0);
	iniStr += generateIniWindow("###action-history", 0, 0, 1920, 1080, "0x00000008", 0);
	iniStr += generateIniWindow("###result-viewer", 0, 0, 1920, 1080, "0x00000003", 0);
	iniStr += generateIniWindow("Dear ImGui Demo", 0, 0, 1920, 1080, "0x00000003", 1);

	iniStr += "[Docking][Data]\n"
		"DockSpace     ID=" + rootDockId + " Window=0xA787BDB4 Pos=0,0 Size=1920,1080 Split=Y Selected=0x25A319F0\n"
		"  DockNode    ID=0x00000001 Parent=" + rootDockId + " SizeRef=1920,259 Split=X Selected=0xEBE6C6E6\n"
		"    DockNode    ID=0x00000002 Parent=0x00000001 SizeRef=1587,259 Split=X Selected=0xEBE6C6E6\n"
		"      DockNode  ID=0x00000007 Parent=0x00000002 SizeRef=1187,537 Selected=0xEBE6C6E6\n"
		"      DockNode  ID=0x00000008 Parent=0x00000002 SizeRef=400,537 Selected=0xEBE6C6E6\n"
		"    DockNode  ID=0x00000003 Parent=0x00000001 SizeRef=331,537 Selected=0x25A319F0\n"
		"  DockNode    ID=0x00000004 Parent=" + rootDockId + " SizeRef=1920,819 Split=X Selected=0x2DD179D7\n"
		"    DockNode  ID=0x00000005 Parent=0x00000004 SizeRef=960,819 Selected=0x2DD179D7\n"
		"    DockNode  ID=0x00000006 Parent=0x00000004 SizeRef=960,819 Selected=0x2DD179D7\n"
		"\n";

	return iniStr;
}

namespace {
tstudio::App* app = nullptr;
tstudio::TextureId image_texture_id;
static bool reloadLayout = true;
static NodeModule nodeModule = NodeModule();
static NodeModule nodeModuleSecondary = NodeModule();
static MonitorModule viewerModule = MonitorModule();
static ActionHistoryModule actionHistoryModule = ActionHistoryModule();

static void post_imgui_init(const tstudio::blank_callbacks& callbacks) {
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

	nodeModule.onMount();
	nodeModuleSecondary.onMount();
	viewerModule.onMount();
}

void on_blank(const tstudio::blank_callbacks& callbacks) {
	if (reloadLayout) {
		std::string iniCommands = generateIniCommands();
		ImGui::LoadIniSettingsFromMemory(iniCommands.c_str());
		reloadLayout = false;
	}
	
	app->onBlank(callbacks);

	viewerModule.onBlank(app, callbacks);
}

static void pre_imgui_destory(const tstudio::blank_callbacks& callbacks) {
	callbacks.destory_texture(image_texture_id);
	ImNodes::DestroyContext();
}

static void final_destory() {
	delete app;
}


} // namespace

void run_base(App* appPtr) {
	app = appPtr;
	render_hooks hooks;
	hooks.render_frame = main_loop;
	hooks.on_blank = on_blank;
	hooks.post_imgui_init = post_imgui_init;
	hooks.pre_imgui_destory = pre_imgui_destory;
	hooks.final_destory = final_destory;
	glfw_run(hooks);
}

static void main_loop() {
	// Our state
	static bool show_demo_window = true;
	static bool show_another_window = true;
	static bool show_second_node_editor = false;
	static bool show_action_history = true;
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	ImGui::DockSpaceOverViewport(nullptr, 0, nullptr);

	{
		ImGui::Begin("Node-Editor###node-viewer");

		nodeModule.render(app);

		ImGui::End();
	}

	if (show_second_node_editor) {

		ImGui::Begin("Node-Editor (Secondary)###node-viewer2", &show_second_node_editor);

		nodeModuleSecondary.render(app);

		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_action_history)
	{
		ImGui::Begin("Action Hisotry###action-history", &show_action_history);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		actionHistoryModule.render(app);

		ImGui::End();
	}

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;


		
		ImGui::Begin("Welcome!###splash-screen");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);
		ImGui::Checkbox("Second Nodes", &show_second_node_editor);

		// ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		// ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		{
			ImGui::Text("Runner-Metrics");
			auto metrics = app->getRunnerMetrics();
			ImGui::Text("queueSize=%lu cacheItemCount=%lu cacheMemoryUsed=%luMiB cacheMemoryMax=%luMiB", 
				metrics.queueSize, metrics.cacheItemCount, metrics.cacheMemoryUsed/(1024*1024), metrics.cacheMemoryMax/(1024*1024));

			if (ImGui::Button("Clear Cache")) {
				app->clearRunnerCache();
			}
			if (metrics.clearingCache) {
				ImGui::SameLine();
				ImGui::TextUnformatted("Clearing...");
			}
		}

		if (ImGui::Button("Reset Layout"))
			reloadLayout = true;

		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Viewer###result-viewer", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		viewerModule.render(app);

		ImGui::End();
	}

	auto draw = ImGui::GetBackgroundDrawList();
	draw->AddRect(ImVec2(0, 0), ImVec2(10, 10), 0, 0, 0, 2);

}

} // namespace tstudio