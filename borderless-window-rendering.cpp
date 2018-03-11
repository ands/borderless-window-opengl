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
}

static void handle_paint(struct window *data)
{
	static bool show_demo_window = true;
	static bool show_another_window = false;
	static ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	
	ImGui_ImplGL2_NewFrame(data->width, data->height, data->width, data->height);
	
	// 1. Show a simple window.
	// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
	{
		static float f = 0.0f;
		static int counter = 0;
		ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}

	// 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name your windows.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}

	// 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
	if (show_demo_window)
	{
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
		ImGui::ShowDemoWindow(&show_demo_window);
	}

	// Rendering
	glViewport(0, 0, data->width, data->height);
	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui::Render();
	ImGui_ImplGL2_RenderDrawData(ImGui::GetDrawData());
	SwapBuffers(GetDC(data->hwnd));
}

bool handle_message(struct window *data, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_LBUTTONDOWN && !ImGui::GetIO().WantCaptureMouse)
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