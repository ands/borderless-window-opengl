// Incomplete WinAPI ImGui binding with OpenGL (legacy, fixed pipeline) without any user input.

// Implemented features:
//  [X] User texture binding. Cast 'GLuint' OpenGL texture identifier as void*/ImTextureID. Read the FAQ about ImTextureID in imgui.cpp.

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the opengl3_example/ folder**
// See imgui_impl.cpp for details.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

IMGUI_API bool        ImGui_ImplGL2_Init(HWND window);
IMGUI_API void        ImGui_ImplGL2_Shutdown();
IMGUI_API void        ImGui_ImplGL2_NewFrame(int w, int h, int display_w, int display_h);
IMGUI_API bool        ImGui_ImplGL2_Handle_Message(UINT msg, WPARAM wparam, LPARAM lparam);
IMGUI_API void        ImGui_ImplGL2_RenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void        ImGui_ImplGL2_InvalidateDeviceObjects();
IMGUI_API bool        ImGui_ImplGL2_CreateDeviceObjects();
