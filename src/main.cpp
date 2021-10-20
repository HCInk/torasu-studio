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
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#if BACKEND_MODE == 0 // glfw/opengl
#ifdef __EMSCRIPTEN__
	#define GLFW_INCLUDE_ES3
#endif
	#include <backends/imgui_impl_glfw.h>
	#include <backends/imgui_impl_opengl3.h>
	#if defined(IMGUI_IMPL_OPENGL_ES2)
	#include <GLES2/gl2.h>
	#endif
	#include <GLFW/glfw3.h> // Will drag system OpenGL headers
	static auto ImGui_ImplGlfw_Init = ImGui_ImplGlfw_InitForOpenGL;
	static auto ImGui_ImplGraphics_NewFrame = ImGui_ImplOpenGL3_NewFrame;
	static auto ImGui_ImplGraphics_Shutdown = ImGui_ImplOpenGL3_Shutdown;
	#define GRAPHICS_MODE 0 // opengl
#elif BACKEND_MODE == 1 // wasm/webgpu
	#include <backends/imgui_impl_glfw.h>
	#include <backends/imgui_impl_wgpu.h>
	#include <stdio.h>
	#include <emscripten.h>
	#include <emscripten/html5.h>
	#include <emscripten/html5_webgpu.h>
	#include <GLFW/glfw3.h>
	#include <webgpu/webgpu.h>
	#include <webgpu/webgpu_cpp.h>
	static auto ImGui_ImplGlfw_Init = ImGui_ImplGlfw_InitForOther;
	static auto ImGui_ImplGraphics_NewFrame = ImGui_ImplWGPU_NewFrame;
	static auto ImGui_ImplGraphics_Shutdown = ImGui_ImplWGPU_Shutdown;
	#define GRAPHICS_MODE 1 // webgpu

	// Global WebGPU required states
	static WGPUDevice    wgpu_device = NULL;
	static WGPUSurface   wgpu_surface = NULL;
	static WGPUSwapChain wgpu_swap_chain = NULL;
	static int           wgpu_swap_chain_width = 0;
	static int           wgpu_swap_chain_height = 0;

	// States tracked across render frames
	static bool show_demo_window = true;
	static bool show_another_window = false;
	static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// Forward declarations
	static bool init_wgpu();
	static void print_wgpu_error(WGPUErrorType error_type, const char* message, void*)
	{
		const char* error_type_lbl = "";
		switch (error_type)
		{
		case WGPUErrorType_Validation:  error_type_lbl = "Validation"; break;
		case WGPUErrorType_OutOfMemory: error_type_lbl = "Out of memory"; break;
		case WGPUErrorType_Unknown:     error_type_lbl = "Unknown"; break;
		case WGPUErrorType_DeviceLost:  error_type_lbl = "Device lost"; break;
		default:                        error_type_lbl = "Unknown";
		}
		printf("%s error: %s\n", error_type_lbl, message);
	}


	static bool init_wgpu() {
		wgpu_device = emscripten_webgpu_get_device();
		if (!wgpu_device)
			return false;

		wgpuDeviceSetUncapturedErrorCallback(wgpu_device, print_wgpu_error, NULL);

		// Use C++ wrapper due to misbehavior in Emscripten.
		// Some offset computation for wgpuInstanceCreateSurface in JavaScript
		// seem to be inline with struct alignments in the C++ structure
		wgpu::SurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
		html_surface_desc.selector = "#canvas";

		wgpu::SurfaceDescriptor surface_desc = {};
		surface_desc.nextInChain = &html_surface_desc;

		// Use 'null' instance
		wgpu::Instance instance = {};
		wgpu_surface = instance.CreateSurface(&surface_desc).Release();

		return true;
	}
#endif

static void main_loop();

// #include "../thirdparty/imgui-node-editor/imgui_node_editor.h"
// namespace nodes = ax::NodeEditor;
#include "../thirdparty/imnodes/imnodes.h"

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


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

void updateTexture(GLuint textureId, uint32_t texWidth, uint32_t texHeight, size_t var) {
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

    glBindTexture(GL_TEXTURE_2D, textureId);
    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);

}

namespace {
static const uint32_t baseTexWidth = 1920;
static const uint32_t baseTexHeight = 1080;
static float sizeFactor = 1;
static uint32_t texWidth = baseTexWidth*sizeFactor;
static uint32_t texHeight = baseTexHeight*sizeFactor;
static GLFWwindow* window;
GLuint image_texture;
} // namespace


int main(int, char**) {
	std::cout << "Spwan" << std::endl;
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
#if GRAPHICS_MODE == 0 // opengl
// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif
#elif GRAPHICS_MODE == 1 // webgpu
    // Make sure GLFW does not initialize any graphics context.
    // This needs to be done explicitly later
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

	std::cout << "Create window..." << std::endl;
    // Create window with graphics context
    window = glfwCreateWindow(1920, 1080, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return 1;

#if GRAPHICS_MODE == 1 // webgpu
	// Initialize the WebGPU environment
	std::cout << "Setup WebGPU..." << std::endl;
    if (!init_wgpu())
    {
        if (window)
            glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    glfwShowWindow(window);
#endif

	std::cout << "Configuring window..." << std::endl;
    glfwMakeContextCurrent(window);
#ifndef __EMSCRIPTEN__ // Only local (for web after loop-registration)
	glfwSwapInterval(1); // Enable vsync
#endif

	std::cout << "Setup ImGui..." << std::endl;
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
#ifndef __EMSCRIPTEN__ // No viewports in web
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

	std::cout << "Setup Platform stuff..." << std::endl;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_Init(window, true);
#if GRAPHICS_MODE == 0 // opengl
    ImGui_ImplOpenGL3_Init(glsl_version);
#elif GRAPHICS_MODE == 1 // wgpu
	ImGui_ImplWGPU_Init(wgpu_device, 3, WGPUTextureFormat_RGBA8Unorm);
#endif
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

	std::cout << "Preparing content..." << std::endl;

	ImNodes::CreateContext();

	for (int nodeId = 0; nodeId < 10; nodeId++) {
		ImVec2 position = ImVec2(nodeId*200.0f+100.0f, 100.0f);
		ImNodes::SetNodeGridSpacePos(nodeId, position);
	}

	// Create a OpenGL texture identifier
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	updateTexture(image_texture, texWidth, texHeight, 0);

#ifdef __EMSCRIPTEN__ // Web
	std::cout << "Start web-loop." << std::endl;
	emscripten_set_main_loop(main_loop, 0, false);
    glfwSwapInterval(1); // Enable vsync (has been postponed until now)
	emscripten_run_script("Module.onInitComplete()");
#else // Local
	std::cout << "Start local loop." << std::endl;
    // Main loop
    while (!glfwWindowShouldClose(window)) {
		main_loop();
    }

	ImNodes::DestroyContext();
    // Cleanup
    ImGui_ImplGraphics_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
#endif
    return 0;
}

static void main_loop() {
		// Our state
		static bool show_demo_window = true;
		static bool show_another_window = true;
		static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		static  std::vector<std::pair<int, int>> links;
		static bool reloadLayout = true;
		static float someFloat = 0;
		static int selectNode = -1;
		static bool nodeOpen = true;

		if (reloadLayout) {
			std::string iniCommands = generateIniCommands();
			ImGui::LoadIniSettingsFromMemory(iniCommands.c_str());
			reloadLayout = false;
		}

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplGraphics_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

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


			// nodes::SetCurrentEditor(nodeEditor);

			// nodes::Begin("My Editor");

			// int uniqueId = 1;

			// // Start drawing nodes.
			// for (size_t i = 0; i < 10; i++) {

			// 	nodes::BeginNode(uniqueId++);
			// 		ImGui::Text("Node A");
			// 		nodes::BeginPin(uniqueId++, nodes::PinKind::Input);
			// 			ImGui::Text("-> In");
			// 		nodes::EndPin();
			// 		ImGui::SameLine();
			// 		nodes::BeginPin(uniqueId++, nodes::PinKind::Output);
			// 			ImGui::Text("Out ->");
			// 		nodes::EndPin();
			// 	nodes::EndNode();

			// }

			// nodes::End();


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
			auto texId = (void*)(intptr_t)image_texture;
			ImGui::Image(texId, max);
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
						updateTexture(image_texture, texWidth, texHeight, 0);
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

        // Rendering
        ImGui::Render();
#if BACKEND_MODE == 0
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#elif BACKEND_MODE == 1
		WGPURenderPassColorAttachment color_attachments = {};
		color_attachments.loadOp = WGPULoadOp_Clear;
		color_attachments.storeOp = WGPUStoreOp_Store;
		color_attachments.clearColor = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		color_attachments.view = wgpuSwapChainGetCurrentTextureView(wgpu_swap_chain);
		WGPURenderPassDescriptor render_pass_desc = {};
		render_pass_desc.colorAttachmentCount = 1;
		render_pass_desc.colorAttachments = &color_attachments;
		render_pass_desc.depthStencilAttachment = NULL;

		WGPUCommandEncoderDescriptor enc_desc = {};
		WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(wgpu_device, &enc_desc);

		WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
		ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
		wgpuRenderPassEncoderEndPass(pass);

		WGPUCommandBufferDescriptor cmd_buffer_desc = {};
		WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
		WGPUQueue queue = wgpuDeviceGetQueue(wgpu_device);
		wgpuQueueSubmit(queue, 1, &cmd_buffer);
#endif

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    	ImGuiIO& io = ImGui::GetIO(); (void)io;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
}