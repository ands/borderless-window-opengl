#include <windows.h>
#include "borderless-window.h"
#include "glad.h"
#include "imgui.h"
#include "imgui_window.h"

// Style Editor Window
static void imgui_style_editor_draw(borderless_window_t *window, void * /*userdata*/)
{
	if (!imgui_window_begin("Style Editor"))
		borderless_window_close(window);
	ImGui::ShowStyleEditor();
	imgui_window_end();
}
static void imgui_style_editor_close(borderless_window_t *window, void * /*userdata*/)
{
	borderless_window_close_all(window);
	PostQuitMessage(0);
}
static void imgui_style_editor_create()
{
	imgui_window_create(L"Hello", 1280, 800, imgui_style_editor_draw, imgui_style_editor_close, NULL);
}

// Self-Replicating Test Window
static void imgui_test_create();
static void imgui_test_draw(borderless_window_t *window, void * /*userdata*/)
{
	if (!imgui_window_begin("Imgui!"))
		borderless_window_close(window);

	if (ImGui::Button("Create Window"))
		imgui_test_create();

	ImGui::Text("Window State: %s", window->maximized ? "Maximized" : "Regular");
	ImGui::Text("Composition: %s", window->composition_enabled ? "Enabled" : "Disabled");
	ImGui::Text("Window Size: %d x %d", window->width, window->height);

	imgui_window_end();
}
static void imgui_test_create()
{
	imgui_window_create(L"World", 640, 480, imgui_test_draw, NULL, NULL);
}

// Main
int CALLBACK wWinMain(HINSTANCE /*inst*/, HINSTANCE /*prev*/, LPWSTR /*cmd*/, int /*show*/)
{
	imgui_window_init(3, 1);
	imgui_style_editor_create();
	imgui_test_create();
	int result = imgui_window_message_loop();
	imgui_window_shutdown();
	return result;
}