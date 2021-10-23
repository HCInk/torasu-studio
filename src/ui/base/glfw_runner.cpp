#include "glfw_runner.hpp"

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
	#include "opengl_textures.cpp"
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

namespace {

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static tstudio::render_hooks hooks;
static GLFWwindow* window = nullptr;
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
static tstudio::blank_callbacks blank_callbacks_instance = {
	.create_texture = tstudio::opengl_textures_create_texture,
	.update_texture = tstudio::opengl_textures_update_texture,
	.destory_texture = tstudio::opengl_textures_destory_texture,
	.tex_id_to_imgui_id = tstudio::opengl_textures_tex_id_to_imgui_id
	// TextureId (*create_texture)(uint32_t texWidth, uint32_t texHeight, uint8_t* data);
	// void (*update_texture)(TextureId id, uint32_t texWidth, uint32_t texHeight, uint8_t* data);
	// void (*destory_texture)(TextureId id);
};

void render_frame() {
	

	hooks.on_blank(blank_callbacks_instance);

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
	hooks.render_frame();

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

} // namespace

namespace tstudio {

void glfw_run(const render_hooks& setHooks) {
	hooks = setHooks;
	std::cout << "Spwan" << std::endl;
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        throw std::runtime_error("Failed to init GLFW");
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
        throw std::runtime_error("Failed to create glfw-window");

#if GRAPHICS_MODE == 1 // webgpu
	// Initialize the WebGPU environment
	std::cout << "Setup WebGPU..." << std::endl;
    if (!init_wgpu())
    {
        if (window)
            glfwDestroyWindow(window);
        glfwTerminate();
        throw std::runtime_error("Failed to init WebGPU");
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

	std::cout << "Preparing content..." << std::endl;

	hooks.post_imgui_init(blank_callbacks_instance);

#ifdef __EMSCRIPTEN__ // Web
	std::cout << "Start web-loop." << std::endl;
	emscripten_set_main_loop(render_frame, 0, false);
    glfwSwapInterval(1); // Enable vsync (has been postponed until now)
	emscripten_run_script("Module.onInitComplete()");
#else // Local
	std::cout << "Start local loop." << std::endl;
    // Main loop
    while (!glfwWindowShouldClose(window)) {
		render_frame();
    }

	hooks.pre_imgui_destory(blank_callbacks_instance);

	// Cleanup
	ImGui_ImplGraphics_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
#endif
}

} // namespace tstudio