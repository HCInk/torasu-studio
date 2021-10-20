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
static const uint32_t baseTexWidth = 1920;
static const uint32_t baseTexHeight = 1080;
static float sizeFactor = 1;
static uint32_t texWidth = baseTexWidth*sizeFactor;
static uint32_t texHeight = baseTexHeight*sizeFactor;
tstudio::TextureId image_texture_id;
void* image_texture;
static bool reloadLayout = true;
static bool reloadTexture = false;

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

	for (int nodeId = 0; nodeId < 10; nodeId++) {
		ImVec2 position = ImVec2(nodeId*200.0f+100.0f, 100.0f);
		ImNodes::SetNodeGridSpacePos(nodeId, position);
	}

	auto generated = generateTexture(texWidth, texHeight);
	image_texture_id = callbacks.create_texture(texWidth, texHeight, generated.get());
	image_texture = callbacks.tex_id_to_imgui_id(image_texture_id);
}

void on_blank(const tstudio::render_hooks::blank_callbacks& callbacks) {
	if (reloadLayout) {
		std::string iniCommands = generateIniCommands();
		ImGui::LoadIniSettingsFromMemory(iniCommands.c_str());
		reloadLayout = false;
	}
	if (reloadTexture) {
		auto generated = generateTexture(texWidth, texHeight);
		callbacks.update_texture(image_texture_id, texWidth, texHeight, generated.get());
		reloadTexture = false;
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
	static  std::vector<std::pair<int, int>> links;
	static float someFloat = 0;
	static int selectNode = -1;
	static bool nodeOpen = true;

	ImGui::DockSpaceOverViewport(nullptr, 0, nullptr);

	{
		auto isLinked = [] (int attrId, int* otherLink) {
			for (auto link : links) {
				if (link.first == attrId) {
					*otherLink = link.second;
					return true;
				}
				if (link.second == attrId) {
					*otherLink = link.first;
					return true;
				}
			}
			return false;
		};
		ImGui::Begin("Node-Editor###node-viewer");

		ImNodes::BeginNodeEditor();

		int attrId = 0;
		for (int nodeId = 0; nodeId < 10; nodeId++) {

			ImNodes::BeginNode(nodeId);

			ImNodes::BeginNodeTitleBar();
			ImGui::Checkbox("", &nodeOpen);
			ImGui::SameLine();
			ImGui::TextUnformatted("Test Node");
			ImNodes::EndNodeTitleBar();

			ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkCreationOnSnap);
			ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
			if (nodeOpen) {
				ImVec2 widthSpacer = {150.0, 0.0};
				ImGui::Dummy(widthSpacer);
			}

			for (int i = 0; i < 4; i++) {
				ImNodes::BeginInputAttribute(attrId++);
				
				if (nodeOpen) {
					ImGui::Text("in %i", i);
					ImGui::SameLine();
					int linkedTo;
					if (isLinked(attrId-1, &linkedTo)) {
						int destNodeId = linkedTo/5;
						ImGui::Text("[LINKED to %i]", destNodeId);
						if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0)) {
							selectNode = destNodeId;
						}
					} else {
						ImGui::PushItemWidth(100.0f);
						ImGui::DragFloat("", &someFloat, 0.01, -30.0f, 30.0f, "%.03f");
						ImGui::PopItemWidth();
					}
				}
				ImNodes::EndInputAttribute();

				auto size = ImGui::GetItemRectSize();

				if (i == 0) {
					ImNodes::PopAttributeFlag();
					ImGui::SameLine();
					ImNodes::BeginOutputAttribute(attrId++);
					ImNodes::EndOutputAttribute();
					ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
				}

				if (nodeOpen) {
					float addedHeight = std::max(15.0f-size.y, 0.0f);
					ImGui::SetCursorPosY(ImGui::GetCursorPosY()+addedHeight);
					// std::cout << "size " << nodeId << "-" << i << ": " << " " << addedHeight << std::endl;
				}
			}

			ImNodes::PopAttributeFlag();
			ImNodes::PopAttributeFlag();

			ImNodes::EndNode();
		}

		for (size_t i = 0; i < links.size(); ++i) {
			const std::pair<int, int> p = links[i];
			ImNodes::Link(i, p.first, p.second);
		}

		ImNodes::MiniMap();
		ImNodes::EndNodeEditor();
		int linkId;
		if (ImNodes::IsLinkDestroyed(&linkId)) {
			links.erase(links.begin()+linkId);
		}
		int start_attr, end_attr;
		if (ImNodes::IsLinkCreated(&start_attr, &end_attr)) {
			links.push_back(std::make_pair(start_attr, end_attr));
		}
		if (selectNode >= 0) {
			ImNodes::ClearNodeSelection();
			ImNodes::SelectNode(selectNode);
			ImVec2 selectedPos = ImNodes::GetNodeGridSpacePos(selectNode);
			selectedPos.x *= -1;
			selectedPos.y *= -1;
			selectedPos.x += 200.0;
			selectedPos.y += 200.0;
			ImNodes::EditorContextResetPanning(selectedPos);
			selectNode = -1;
		}

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
		ImGui::Text("Hello from another window!");
		auto max = ImGui::GetContentRegionMax();
		max.y -= 70;
		if (max.y < 0) max.y = 0;
		float ratio = static_cast<float>(texWidth)/texHeight;
		float maxWidth = max.y * ratio;
		float maxHeight = max.x / ratio;
		float sidePadding = 0;
		if (maxWidth <= max.x) {
			sidePadding = (max.x-maxWidth)/2;
			max.x = maxWidth;
		} else {
			max.y = maxHeight;
		}
		ImGui::SetCursorPosX(ImGui::GetCursorPosX()+sidePadding);
		ImGui::Image(image_texture, max);
		if (!ImGui::IsWindowDocked()) {
			if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home))) {
				ImVec2 veiwportSize = ImGui::GetMainViewport()->Size;
				ImVec2 defPos = {veiwportSize.x*1/3, veiwportSize.y*1/3};
				ImVec2 defSize = {veiwportSize.x*1/3, veiwportSize.y*1/3};
				ImGui::SetWindowPos(defPos);
				ImGui::SetWindowSize(defSize);
			}
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				auto mousePos = ImGui::GetMousePos();
				auto windowPos = ImGui::GetWindowPos();
				auto windowSize = ImGui::GetWindowSize();
				ImVec2 posRel = {
					(mousePos.x-windowPos.x)/windowSize.x,
					(mousePos.y-windowPos.y)/windowSize.y,
				};

				static const float zoomFactor = 1.5;

				float currZoomFactor;
				
				if (ImGui::GetIO().KeyShift) {
					currZoomFactor = zoomFactor;
				} else {
					currZoomFactor = 1/zoomFactor;
				}

				const ImVec2 toAdd = {windowSize.x*(currZoomFactor-1), windowSize.y*(currZoomFactor-1)};

				windowPos.x -= toAdd.x*posRel.x;
				windowPos.y -= toAdd.y*posRel.y;
				windowSize.x += toAdd.x;
				windowSize.y += toAdd.y;

				if (windowSize.x > 100 && windowSize.y > 150) {
					ImGui::SetWindowPos(windowPos);
					ImGui::SetWindowSize(windowSize);
				}

			}

		}

		if (ImGui::Button("Close Me"))
			show_another_window = false;

		ImGui::SameLine();

		const char* items[] = { "scale x0.25", "scale x0.5", "scale x1", "scale x2", "scale x4"};
		float factors[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
		static int item_current_idx = 2; // Here we store our selection data as an index.
		const char* combo_preview_value = items[item_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)
		ImGui::PushItemWidth(100.0f);
		if (ImGui::BeginCombo(" ", combo_preview_value))
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				const bool is_selected = (item_current_idx == n);
				if (ImGui::Selectable(items[n], is_selected)) {
					item_current_idx = n;
					sizeFactor = factors[item_current_idx];
					texWidth = baseTexWidth*sizeFactor;
					texHeight = baseTexHeight*sizeFactor;
					reloadTexture = true;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();
		ImGui::Text("%u x %u", texWidth, texHeight);


		ImGui::End();
	}

	auto draw = ImGui::GetBackgroundDrawList();
	draw->AddRect(ImVec2(0, 0), ImVec2(10, 10), 0, 0, 0, 2);

}

} // namespace tstudio