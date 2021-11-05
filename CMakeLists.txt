cmake_minimum_required(VERSION 3.0)

option(WEB_MODE "Web Mode" OFF)
if (WEB_MODE)
	set(CMAKE_TOOLCHAIN_FILE "/usr/lib/emscripten/cmake/Modules/Platform/Emscripten.cmake" )
	set(CMAKE_CROSSCOMPILING_EMULATOR "/usr/bin/node")
endif()

project(torasu-studio VERSION 0.0.4)
set(CMAKE_CXX_STANDARD 20)

include_directories(BEFORE ${CMAKE_CURRENT_LIST_DIR}/thirdparty/imgui)

if (EMSCRIPTEN)
	include_directories(BEFORE /usr/lib/emscripten/system/include/)
	message("Compiling for web")
else()
	message("Compiling for local")
endif()

add_executable(torasu-studio 
	${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/state/App.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/state/TreeManager.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/state/RenderQueue.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/state/monitors/Monitor.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/state/monitors/NumberMonitor.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/state/monitors/ImageMonitor.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/ui/base/base.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/ui/base/glfw_runner.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/ui/modules/NodeModule.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/ui/modules/ViewerModule.cpp
	${CMAKE_CURRENT_LIST_DIR}/thirdparty/imgui/imgui.cpp
	${CMAKE_CURRENT_LIST_DIR}/thirdparty/imgui/imgui_draw.cpp
	${CMAKE_CURRENT_LIST_DIR}/thirdparty/imgui/imgui_tables.cpp
	${CMAKE_CURRENT_LIST_DIR}/thirdparty/imgui/imgui_widgets.cpp
	${CMAKE_CURRENT_LIST_DIR}/thirdparty/imgui/imgui_demo.cpp
	${CMAKE_CURRENT_LIST_DIR}/thirdparty/imgui/backends/imgui_impl_glfw.cpp
	${CMAKE_CURRENT_LIST_DIR}/thirdparty/imgui/backends/imgui_impl_opengl3.cpp
	${CMAKE_CURRENT_LIST_DIR}/thirdparty/imnodes/imnodes.cpp
)

if (EMSCRIPTEN)
	# set(CMAKE_EXECUTABLE_SUFFIX ".html")
	set(CMAKE_EXECUTABLE_SUFFIX ".wasm.js")
	set_target_properties(torasu-studio PROPERTIES LINK_FLAGS "-Os -s ASSERTIONS=1 -s FETCH=1 -s USE_FREETYPE=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s WASM=1 -s PTHREAD_POOL_SIZE=64 -fexceptions -s EXPORTED_FUNCTIONS='[_main]'")
	set_target_properties(torasu-studio PROPERTIES COMPILE_FLAGS "-fexceptions")
	target_link_directories(torasu-studio PUBLIC ~/.emscripten_cache/sysroot/lib)
	target_link_libraries(torasu-studio torasu torasustd imgc)
	configure_file(${CMAKE_CURRENT_LIST_DIR}/src/index.html index.html COPYONLY)
	# target_link_libraries(torasu-studio glfw GL dl)
else()
	set_target_properties(torasu-studio PROPERTIES LINK_FLAGS "-Wl,-rpath,/usr/local/lib")
	target_link_libraries(torasu-studio asan glfw GL dl torasu torasustd imgc)
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# target_link_libraries(cadl torasu torasustd imgc lodepng)

