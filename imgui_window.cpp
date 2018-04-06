static int g_openglMajorVersion;
static int g_openglMinorVersion;
static HGLRC hglrc; // Global, shared between windows

struct imgui_window_t;

typedef void (*imgui_window_func)(borderless_window_t *window, void *userdata);

struct imgui_window_t
{
	ImGuiContext *context;
	imgui_window_func func;
	void *userdata;
};

bool imgui_window_begin(const char *title)
{
	bool show = true;
	return ImGui::Begin(title, &show, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse) && show;
}
void imgui_window_end()
{
	ImGui::End();
}

static bool imgui_message(borderless_window_t *window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	ImGui::SetCurrentContext(((imgui_window_t*)window->userdata)->context);
	
	if (HIWORD(lparam) < 19 && LOWORD(lparam) < window->width - 17) // TODO: Get title bar dimensions from imgui
	{
		if (msg == WM_LBUTTONDOWN  ) { SendMessageW(window->hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);             return true; } // Drag window
		if (msg == WM_LBUTTONDBLCLK) { ShowWindow(window->hwnd, window->maximized ? SW_RESTORE : SW_MAXIMIZE); return true; } // Toggle maximize
	}
	if (msg == WM_PAINT)
	{
		wglMakeCurrent(window->hdc, hglrc);
		ImGui_Impl_WinAPI_GL3_NewFrame(window->hwnd, window->width, window->height, window->width, window->height);
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImVec2((float)window->width, (float)window->height));
		((imgui_window_t*)window->userdata)->func(window, ((imgui_window_t*)window->userdata)->userdata);
		glViewport(0, 0, window->width, window->height);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui::Render();
		ImGui_Impl_WinAPI_GL3_RenderDrawData(ImGui::GetDrawData());
		SwapBuffers(window->hdc);
		return true;
	}
	if (msg == WM_CLOSE || msg == WM_QUIT)
	{
		ImGui_Impl_WinAPI_GL3_Shutdown();
		ImGui::DestroyContext();
		free((imgui_window_t*)window->userdata);
		return true;
	}
	
	return ImGui_Impl_WinAPI_GL3_Handle_Message(window->hwnd, msg, wparam, lparam);
}

static BOOL CALLBACK imgui_enum_thread_window(HWND hwnd, LPARAM lParam)
{
	if (hwnd != (HWND)lParam)
		UpdateWindow(hwnd);
	return TRUE;
}

borderless_window_t * imgui_window_create(LPCWSTR title, int w, int h, imgui_window_func func, void *userdata)
{
	ImGuiContext* previous = ImGui::GetCurrentContext();

	imgui_window_t *imgui = (imgui_window_t*)calloc(1, sizeof(imgui_window_t));
	imgui->func = func;
	imgui->userdata = userdata;
	borderless_window_t* window = borderless_window_create(title, w, h, imgui_message, imgui);

	if (!hglrc)
	{
		if (!(hglrc = opengl_create_context(window->hdc, g_openglMajorVersion, g_openglMinorVersion)))
			ExitProcess(ERROR_INVALID_HANDLE);
		gladLoadGL();
	}
	else
	{
		if (!opengl_set_pixelformat(window->hdc))
			ExitProcess(ERROR_INVALID_HANDLE);
	}

	ImGui::SetCurrentContext((ImGuiContext*)(((imgui_window_t*)window->userdata)->context = ImGui::CreateContext()));
	ImGui_Impl_WinAPI_GL3_Init();
	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 0.0f; // Try to hide remaining 1px row of windows border in the corners which needs to be there to not get other artifacts :(

	ShowWindow(window->hwnd, SW_SHOWDEFAULT);
	UpdateWindow(window->hwnd);

	EnumThreadWindows(GetWindowThreadProcessId(window->hwnd, 0), imgui_enum_thread_window, (LPARAM)window->hwnd);

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
	opengl_destroy_context(hglrc);
	hglrc = NULL;
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