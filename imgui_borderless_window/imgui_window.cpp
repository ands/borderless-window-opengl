#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include "windows.h"
#include <malloc.h>
#include "borderless_window.h"
#include "glad.h"
#include "opengl_context.h"
#include "imgui.h"
#include "imgui_impl_gl3.h"
#include "imgui_window.h"

static int g_openglMajorVersion;
static int g_openglMinorVersion;
static HGLRC g_hglrc; // Global, shared between windows
static int g_windowIdCounter = 0; // Helps during debugging

struct imgui_context_t
{
	ImGuiContext *context;
	imgui_window_func draw;
	imgui_window_func close;
	void *userdata;
	int id;
};

bool imgui_window_begin(const char *title, ImGuiWindowFlags flags)
{
	bool show = true;
	return ImGui::Begin(title, &show, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | flags) && show;
}
void imgui_window_end()
{
	ImGui::End();
}

static bool imgui_message(imgui_window_t *window, unsigned int msg, unsigned __int64 wparam, unsigned __int64 lparam)
{
	imgui_context_t *imgui = (imgui_context_t*)window->userdata;
	ImGui::SetCurrentContext(imgui->context);
	
	if (HIWORD(lparam) < 19 && LOWORD(lparam) < window->width - 17) // TODO: Get title bar dimensions from imgui
	{
		if (msg == WM_LBUTTONDOWN  ) { SendMessageW((HWND)window->hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);             return true; } // Drag window
		if (msg == WM_LBUTTONDBLCLK) { ShowWindow((HWND)window->hwnd, window->maximized ? SW_RESTORE : SW_MAXIMIZE); return true; } // Toggle maximize
	}
	if (msg == WM_PAINT)
	{
		wglMakeCurrent((HDC)window->hdc, g_hglrc);
		ImGui_Impl_WinAPI_GL3_NewFrame((HWND)window->hwnd, window->width, window->height, window->width, window->height);
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImVec2((float)window->width, (float)window->height));
		imgui->draw(window, imgui->userdata);
		glViewport(0, 0, window->width, window->height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		ImGui_Impl_WinAPI_GL3_RenderDrawData(ImGui::GetDrawData());
		SwapBuffers((HDC)window->hdc);
		return true;
	}
	if (msg == WM_CLOSE || msg == WM_QUIT)
	{
		if (imgui->close)
			imgui->close(window, imgui->userdata);
		ImGui_Impl_WinAPI_GL3_Shutdown();
		ImGui::DestroyContext();
		free(imgui);
		return true;
	}
	
	return ImGui_Impl_WinAPI_GL3_Handle_Message((HWND)window->hwnd, msg, wparam, lparam);
}

imgui_window_t *imgui_window_create(const wchar_t *title, int w, int h, imgui_window_func draw, imgui_window_func close, void *userdata)
{
	ImGuiContext* previous = ImGui::GetCurrentContext();
	HDC previousDC = wglGetCurrentDC();

	imgui_context_t *imgui = (imgui_context_t*)calloc(1, sizeof(imgui_context_t));
	imgui->draw = draw;
	imgui->close = close;
	imgui->userdata = userdata;
	imgui->id = g_windowIdCounter++;
	imgui_window_t* window = borderless_window_create(title, w, h, imgui_message, imgui);

	if (!g_hglrc)
	{
		if (!(g_hglrc = opengl_create_context((HDC)window->hdc, g_openglMajorVersion, g_openglMinorVersion)))
			ExitProcess(ERROR_INVALID_HANDLE);
		gladLoadGL();
	}
	else
	{
		if (!opengl_set_pixelformat((HDC)window->hdc))
			ExitProcess(ERROR_INVALID_HANDLE);
	}

	ImGui::SetCurrentContext((ImGuiContext*)(((imgui_context_t*)window->userdata)->context = ImGui::CreateContext()));
	ImGui_Impl_WinAPI_GL3_Init();
	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 0.0f; // Try to hide remaining 1px row of windows border in the corners which needs to be there to not get other artifacts :(

	ShowWindow((HWND)window->hwnd, SW_SHOWDEFAULT);
	UpdateWindow((HWND)window->hwnd);

	if (previousDC)
		wglMakeCurrent(previousDC, g_hglrc);
	ImGui::SetCurrentContext(previous);

	return window;
}

void imgui_window_init(int openglMajorVersion, int openglMinorVersion)
{
	g_openglMajorVersion = openglMajorVersion;
	g_openglMinorVersion = openglMinorVersion;
	borderless_window_register();
}

void imgui_window_shutdown()
{
	opengl_destroy_context(g_hglrc);
	g_hglrc = NULL;
	borderless_window_unregister();
}

int imgui_window_message_loop()
{
	MSG message = {0};
	while (GetMessageW(&message, NULL, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	return (int)message.wParam;
}

void imgui_window_close_all(imgui_window_t *root)
{
	borderless_window_close_all(root);
}

void imgui_window_close(imgui_window_t *window)
{
	borderless_window_close(window);
}

void imgui_window_quit()
{
	borderless_window_quit();
}
