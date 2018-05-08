cmake_minimum_required (VERSION 2.8)
project(example)

include_directories(
	${PROJECT_SOURCE_DIR}
	imgui
	imgui_borderless_window)

add_executable(${PROJECT_NAME} WIN32
	example.cpp
	imgui/imgui.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui_borderless_window/borderless_window.cpp
	imgui_borderless_window/glad.c
	imgui_borderless_window/opengl_context.cpp
	imgui_borderless_window/imgui_window.cpp
	imgui_borderless_window/imgui_impl_gl3.cpp)
