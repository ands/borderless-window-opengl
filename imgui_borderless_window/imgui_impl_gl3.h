// Incomplete WinAPI ImGui binding with OpenGL 3

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

IMGUI_API bool        ImGui_Impl_WinAPI_GL3_Init();
IMGUI_API void        ImGui_Impl_WinAPI_GL3_Shutdown();
IMGUI_API void        ImGui_Impl_WinAPI_GL3_NewFrame(HWND hwnd, int w, int h, int display_w, int display_h);
IMGUI_API bool        ImGui_Impl_WinAPI_GL3_Handle_Message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
IMGUI_API void        ImGui_Impl_WinAPI_GL3_RenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_Impl_WinAPI_GL3_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_Impl_WinAPI_GL3_CreateDeviceObjects();
