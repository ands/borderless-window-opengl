cmake_minimum_required (VERSION 2.8)
project(example)

include_directories(
	${PROJECT_SOURCE_DIR}
	imgui_winapi_gl3
	imgui_borderless_window)

add_executable(${PROJECT_NAME} WIN32
	example.cpp
	imgui_borderless_window/borderless-window.cpp
	imgui_borderless_window/glad.c
	imgui_borderless_window/opengl_context.cpp
	imgui_borderless_window/imgui_window.cpp
	imgui_winapi_gl3/imgui.cpp
	imgui_winapi_gl3/imgui_demo.cpp
	imgui_winapi_gl3/imgui_draw.cpp
	imgui_winapi_gl3/imgui_impl_gl3.cpp)
