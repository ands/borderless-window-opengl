#include <windows.h>
#include "borderless-window.h"
#include "opengl_context.h"
#include "glad.h"
#include "imgui.h"
#include "imgui_impl_gl3.h"

static HGLRC hglrc; // Global, shared between windows

void init(struct window *data)
{
	ImGui::SetCurrentContext((ImGuiContext*)(data->userdata = ImGui::CreateContext()));
	ImGui_Impl_WinAPI_GL3_Init();
	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 0.0f; // Try to hide remaining 1px row of windows border in the corners which needs to be there to not get other artifacts :(
}

void shutdown(struct window *data)
{
	ImGui::SetCurrentContext((ImGuiContext*)data->userdata);
	ImGui_Impl_WinAPI_GL3_Shutdown();
	ImGui::DestroyContext();
}

static void imgui(struct window *data)
{
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2((float)data->width, (float)data->height));
	bool show = true;
	if (!ImGui::Begin("Borderless OpenGL Window Example", &show, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse) || !show)
		PostQuitMessage(0);

	ImGui::ShowStyleEditor(); // TODO: Replace this with your UI

	ImGui::End();
}

static void paint(struct window *data)
{
	wglMakeCurrent(data->hdc, hglrc);
	ImGui_Impl_WinAPI_GL3_NewFrame(data->hwnd, data->width, data->height, data->width, data->height);
	imgui(data);
	glViewport(0, 0, data->width, data->height);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui::Render();
	ImGui_Impl_WinAPI_GL3_RenderDrawData(ImGui::GetDrawData());
	SwapBuffers(data->hdc);
}

bool handle_message(struct window *data, UINT msg, WPARAM wparam, LPARAM lparam)
{
	ImGui::SetCurrentContext((ImGuiContext*)data->userdata);
	if (HIWORD(lparam) < 19 && LOWORD(lparam) < data->width - 17) // TODO: Get title bar dimensions from imgui
	{
		if (msg == WM_LBUTTONDOWN  ) { SendMessageW(data->hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);           return true; } // Drag window
		if (msg == WM_LBUTTONDBLCLK) { ShowWindow(data->hwnd, data->maximized ? SW_RESTORE : SW_MAXIMIZE); return true; } // Toggle maximize
	}
	if (msg == WM_PAINT) { paint(data); return true; }
	return ImGui_Impl_WinAPI_GL3_Handle_Message(data->hwnd, msg, wparam, lparam);
}

int CALLBACK wWinMain(HINSTANCE /*inst*/, HINSTANCE /*prev*/, LPWSTR /*cmd*/, int /*show*/)
{
	borderless_window_register();
	struct window* window0 = borderless_window_create(L"Hello", 1280, 800, handle_message, NULL);
	struct window* window1 = borderless_window_create(L"World", 640, 480, handle_message, NULL);

	if (!(hglrc = opengl_create_context(window0->hdc)) || !opengl_set_pixelformat(window1->hdc))
		ExitProcess(ERROR_INVALID_HANDLE);
	gladLoadGL();

	init(window0);
	init(window1);

	ShowWindow(window0->hwnd, SW_SHOWDEFAULT);
	UpdateWindow(window0->hwnd);
	ShowWindow(window1->hwnd, SW_SHOWDEFAULT);
	UpdateWindow(window1->hwnd);
	UpdateWindow(window0->hwnd); // Without this one window will show up with a windows border until one of them is moved!?

	MSG message = {0};
	while (GetMessageW(&message, NULL, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}

	shutdown(window0);
	shutdown(window1);

	opengl_destroy_context(hglrc);
	borderless_window_destroy(window0);
	borderless_window_destroy(window1);
	borderless_window_unregister();
	return (int)message.wParam;
}