#include <windows.h>
#include <GL/gl.h>
#include "borderless-window-rendering.h"
#include "imgui.h"
#include "imgui_impl_gl2.h"

void handle_init(struct window *data)
{
	ImGui::CreateContext();
	ImGui_ImplGL2_Init(data->hwnd);
	ImGui::StyleColorsDark();
	
	// Try to hide remaining 1px row of windows border in the corners
	// which needs to be there to not get other artifacts :(
	ImGui::GetStyle().WindowRounding = 0.0f;
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

static void handle_paint(struct window *data)
{
	ImGui_ImplGL2_NewFrame(data->width, data->height, data->width, data->height);
	imgui(data);
	glViewport(0, 0, data->width, data->height);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui::Render();
	ImGui_ImplGL2_RenderDrawData(ImGui::GetDrawData());
	Sleep(15); // Optional. Reduce CPU usage that shows up in the task manager. Should depend on your frame duration and refresh interval!
	SwapBuffers(GetDC(data->hwnd));
}

bool handle_message(struct window *data, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// TODO: Handle double click for maximization!
	// TODO: Get title bar dimensions from imgui for moving the window:
	if (msg == WM_LBUTTONDOWN && HIWORD(lparam) < 18 && LOWORD(lparam) < data->width - 18)
	{
		ReleaseCapture();
		SendMessageW(data->hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0); // Allow window dragging from any point
		return true;
	}
	
	if (msg == WM_PAINT)
	{
		handle_paint(data);
		return true;
	}
	
	return ImGui_ImplGL2_Handle_Message(msg, wparam, lparam);
}

void handle_shutdown(struct window * /*data*/)
{
	ImGui_ImplGL2_Shutdown();
	ImGui::DestroyContext();
}