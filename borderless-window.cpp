/* borderless-window - a minimal borderless window with the Windows API
 *
 * Written in 2016 by James Ross-Gowan, modified in 2018 by Andreas Mantler
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * See <https://creativecommons.org/publicdomain/zero/1.0/> for a copy of the
 * CC0 Public Domain Dedication, which applies to this software.
 */

#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <versionhelpers.h>
#include <stdlib.h>
#include <stdbool.h>
#include "borderless-window-rendering.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "opengl32.lib")

#define HINST_THISCOMPONENT GetModuleHandle(NULL)

#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00AE)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME (0x00AF)
#endif

static void update_region(struct window *data)
{
	RECT old_rgn = data->rgn;
	RECT r = {0};

	if (IsMaximized(data->hwnd)) {
		WINDOWINFO wi = {};
		wi.cbSize = sizeof(wi);
		GetWindowInfo(data->hwnd, &wi);

		/* For maximized windows, a region is needed to cut off the non-client
		   borders that hang over the edge of the screen */
		data->rgn.left = wi.rcClient.left - wi.rcWindow.left;
		data->rgn.top = wi.rcClient.top - wi.rcWindow.top;
		data->rgn.right = wi.rcClient.right - wi.rcWindow.left;
		data->rgn.bottom = wi.rcClient.bottom - wi.rcWindow.top;
	} else if (!data->composition_enabled) {
		/* For ordinary themed windows when composition is disabled, a region
		   is needed to remove the rounded top corners. Make it as large as
		   possible to avoid having to change it when the window is resized. */
		data->rgn.left = 0;
		data->rgn.top = 0;
		data->rgn.right = 32767;
		data->rgn.bottom = 32767;
	} else {
		/* Don't mess with the region when composition is enabled and the
		   window is not maximized, otherwise it will lose its shadow */
		data->rgn = r;
	}

	/* Avoid unnecessarily updating the region to avoid unnecessary redraws */
	if (EqualRect(&data->rgn, &old_rgn))
		return;
	/* Treat empty regions as NULL regions */
	if (EqualRect(&data->rgn, &r))
		SetWindowRgn(data->hwnd, NULL, TRUE);
	else
		SetWindowRgn(data->hwnd, CreateRectRgnIndirect(&data->rgn), TRUE);
}

static void handle_nccreate(HWND window, CREATESTRUCTW *cs)
{
	struct window *data = (struct window*)cs->lpCreateParams;
	SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)data);
}

static void handle_compositionchanged(struct window *data)
{
	BOOL enabled = FALSE;
	DwmIsCompositionEnabled(&enabled);
	data->composition_enabled = enabled == TRUE;

	if (enabled) {
		/* The window needs a frame to show a shadow, so give it the smallest
		   amount of frame possible */
		MARGINS m = {0};
		m.cyTopHeight = 1;
		DwmExtendFrameIntoClientArea(data->hwnd, &m);
		DWORD state = DWMNCRP_ENABLED;
		DwmSetWindowAttribute(data->hwnd, DWMWA_NCRENDERING_POLICY, &state, sizeof(DWORD));
	}

	update_region(data);
}

static bool has_autohide_appbar(UINT edge, RECT mon)
{
	APPBARDATA abd = {};
	abd.cbSize = sizeof(APPBARDATA);
	abd.uEdge = edge;
	if (IsWindows8Point1OrGreater()) {
		abd.rc = mon;
		return SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &abd) == TRUE;
	}

	/* Before Windows 8.1, it was not possible to specify a monitor when
	   checking for hidden appbars, so check only on the primary monitor */
	if (mon.left != 0 || mon.top != 0)
		return false;
	return SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd) == TRUE;
}

static void handle_nccalcsize(struct window *data, WPARAM wparam,
                              LPARAM lparam)
{
	union {
		LPARAM lparam;
		RECT* rect;
	} params = { lparam };

	/* DefWindowProc must be called in both the maximized and non-maximized
	   cases, otherwise tile/cascade windows won't work */
	RECT nonclient = *params.rect;
	DefWindowProcW(data->hwnd, WM_NCCALCSIZE, wparam, params.lparam);
	RECT client = *params.rect;

	if (IsMaximized(data->hwnd)) {
		WINDOWINFO wi = {0};
		wi.cbSize = sizeof(wi);
		GetWindowInfo(data->hwnd, &wi);

		/* Maximized windows always have a non-client border that hangs over
		   the edge of the screen, so the size proposed by WM_NCCALCSIZE is
		   fine. Just adjust the top border to remove the window title. */
		RECT r;
		r.left = client.left;
		r.top = nonclient.top + wi.cyWindowBorders;
		r.right = client.right;
		r.bottom = client.bottom;
		*params.rect = r;

		HMONITOR mon = MonitorFromWindow(data->hwnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = {0};
		mi.cbSize = sizeof(mi);
		GetMonitorInfoW(mon, &mi);

		/* If the client rectangle is the same as the monitor's rectangle,
		   the shell assumes that the window has gone fullscreen, so it removes
		   the topmost attribute from any auto-hide appbars, making them
		   inaccessible. To avoid this, reduce the size of the client area by
		   one pixel on a certain edge. The edge is chosen based on which side
		   of the monitor is likely to contain an auto-hide appbar, so the
		   missing client area is covered by it. */
		if (EqualRect(params.rect, &mi.rcMonitor)) {
			if (has_autohide_appbar(ABE_BOTTOM, mi.rcMonitor))
				params.rect->bottom--;
			else if (has_autohide_appbar(ABE_LEFT, mi.rcMonitor))
				params.rect->left++;
			else if (has_autohide_appbar(ABE_TOP, mi.rcMonitor))
				params.rect->top++;
			else if (has_autohide_appbar(ABE_RIGHT, mi.rcMonitor))
				params.rect->right--;
		}
	} else {
		/* For the non-maximized case, set the output RECT to what it was
		   before WM_NCCALCSIZE modified it. This will make the client size the
		   same as the non-client size. */
		*params.rect = nonclient;
	}
}

static LRESULT handle_nchittest(struct window *data, int x, int y)
{
	if (IsMaximized(data->hwnd))
		return HTCLIENT;

	POINT mouse = { x, y };
	ScreenToClient(data->hwnd, &mouse);

	/* The horizontal frame should be the same size as the vertical frame,
	   since the NONCLIENTMETRICS structure does not distinguish between them */
	int frame_size = GetSystemMetrics(SM_CXFRAME) +
	                 GetSystemMetrics(SM_CXPADDEDBORDER);
	/* The diagonal size handles are wider than the frame */
	int diagonal_width = frame_size * 2 + GetSystemMetrics(SM_CXBORDER);

	if (mouse.y < frame_size) {
		if (mouse.x < diagonal_width)
			return HTTOPLEFT;
		if (mouse.x >= (int)data->width - diagonal_width)
			return HTTOPRIGHT;
		return HTTOP;
	}

	if (mouse.y >= (int)data->height - frame_size) {
		if (mouse.x < diagonal_width)
			return HTBOTTOMLEFT;
		if (mouse.x >= (int)data->width - (int)diagonal_width)
			return HTBOTTOMRIGHT;
		return HTBOTTOM;
	}

	if (mouse.x < frame_size)
		return HTLEFT;
	if (mouse.x >= (int)data->width - frame_size)
		return HTRIGHT;
	return HTCLIENT;
}

static void handle_themechanged(struct window *data)
{
	data->theme_enabled = IsThemeActive() == TRUE;
}

static void handle_windowposchanged(struct window *data, const WINDOWPOS *pos)
{
	RECT client;
	GetClientRect(data->hwnd, &client);
	unsigned old_width = data->width;
	unsigned old_height = data->height;
	data->width = client.right;
	data->height = client.bottom;
	bool client_changed = data->width != old_width || data->height != old_height;

	if (client_changed || (pos->flags & SWP_FRAMECHANGED))
		update_region(data);

	if (client_changed) {
		/* Invalidate the changed parts of the rectangle drawn in WM_PAINT */
		if (data->width > old_width) {
			RECT r = { (LONG)(old_width - 1), (LONG)0, (LONG)old_width, (LONG)old_height };
			InvalidateRect(data->hwnd, &r, TRUE);
		} else {
			RECT r = { (LONG)(data->width - 1), (LONG)0, (LONG)data->width, (LONG)data->height };
			InvalidateRect(data->hwnd, &r, TRUE);
		}
		if (data->height > old_height) {
			RECT r = { (LONG)0, (LONG)(old_height - 1), (LONG)old_width, (LONG)old_height };
			InvalidateRect(data->hwnd, &r, TRUE);
		} else {
			RECT r = { (LONG)0, (LONG)(data->height - 1), (LONG)data->width, (LONG)data->height };
			InvalidateRect(data->hwnd, &r, TRUE);
		}
	}
}

static LRESULT handle_message_invisible(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	LONG_PTR old_style = GetWindowLongPtrW(window, GWL_STYLE);

	/* Prevent Windows from drawing the default title bar by temporarily
	   toggling the WS_VISIBLE style. This is recommended in:
	   https://blogs.msdn.microsoft.com/wpfsdk/2008/09/08/custom-window-chrome-in-wpf/ */
	SetWindowLongPtrW(window, GWL_STYLE, old_style & ~WS_VISIBLE);
	LRESULT result = DefWindowProcW(window, msg, wparam, lparam);
	SetWindowLongPtrW(window, GWL_STYLE, old_style);

	return result;
}

static LRESULT CALLBACK borderless_window_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	struct window *data = (struct window*)GetWindowLongPtrW(window, GWLP_USERDATA);
	if (!data) {
		/* Due to a longstanding Windows bug, overlapped windows will receive a
		   WM_GETMINMAXINFO message before WM_NCCREATE. This is safe to ignore.
		   It doesn't need any special handling anyway. */
		if (msg == WM_NCCREATE)
			handle_nccreate(window, (CREATESTRUCTW*)lparam);
		return DefWindowProcW(window, msg, wparam, lparam);
	}

	switch (msg) {
	case WM_CLOSE:
		DestroyWindow(window);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_DWMCOMPOSITIONCHANGED:
		handle_compositionchanged(data);
		return 0;
	case WM_NCACTIVATE:
		/* DefWindowProc won't repaint the window border if lParam (normally a HRGN) is -1. This is recommended in:
		   https://blogs.msdn.microsoft.com/wpfsdk/2008/09/08/custom-window-chrome-in-wpf/ */
		return DefWindowProcW(window, msg, wparam, -1);
	case WM_NCCALCSIZE:
		handle_nccalcsize(data, wparam, lparam);
		return 0;
	case WM_NCHITTEST:
		return handle_nchittest(data, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
	case WM_NCPAINT:
		/* Only block WM_NCPAINT when composition is disabled. If it's blocked
		   when composition is enabled, the window shadow won't be drawn. */
		if (!data->composition_enabled)
			return 0;
		break;
	case WM_NCUAHDRAWCAPTION:
	case WM_NCUAHDRAWFRAME:
		/* These undocumented messages are sent to draw themed window borders.
		   Block them to prevent drawing borders over the client area. */
		return 0;
	case WM_SETICON:
	case WM_SETTEXT:
		/* Disable painting while these messages are handled to prevent them
		   from drawing a window caption over the client area, but only when
		   composition and theming are disabled. These messages don't paint
		   when composition is enabled and blocking WM_NCUAHDRAWCAPTION should
		   be enough to prevent painting when theming is enabled. */
		if (!data->composition_enabled && !data->theme_enabled)
			return handle_message_invisible(window, msg, wparam, lparam);
		break;
	case WM_SIZE:
		data->minimized = wparam == SIZE_MINIMIZED;
		data->maximized = wparam == SIZE_MAXIMIZED;
		return 0;
	case WM_THEMECHANGED:
		handle_themechanged(data);
		break;
	case WM_WINDOWPOSCHANGED:
		handle_windowposchanged(data, (WINDOWPOS*)lparam);
		return DefWindowProcW(window, msg, wparam, lparam); // Must be executed so that WM_SIZE and WM_MOVE get sent properly!
	}

	if (handle_message(data, msg, wparam, lparam))
		return 0;

	return DefWindowProcW(window, msg, wparam, lparam);
}

// We're just setting up some oldschool opengl context for testing here,
// because setting up a modern opengl context is a PITA.
HGLRC setup_opengl2(HWND window)
{
	HDC hdc = GetDC(window);
	if (!hdc)
	{
		MessageBoxA(window, "GetDC failed", "ERROR", MB_OK);
		return NULL;
	}

	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0, 8, 0,
		0, 0, 0, 0, 0,  // accum
		32,             // zbuffer
		0,              // stencil!
		0,              // aux
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	unsigned int pf = ChoosePixelFormat(hdc, &pfd);
	if (!pf)
	{
		MessageBoxA(window, "ChoosePixelFormat failed", "ERROR", MB_OK);
		return NULL;
	}

	if (!SetPixelFormat(hdc, pf, &pfd))
	{
		MessageBoxA(window, "SetPixelFormat failed", "ERROR", MB_OK);
		return NULL;
	}

	HGLRC hglrc = wglCreateContext(hdc);
	if (!hglrc)
	{
		MessageBoxA(window, "wglCreateContext failed", "ERROR", MB_OK);
		return NULL;
	}

	if (!wglMakeCurrent(hdc, hglrc))
	{
		wglDeleteContext(hglrc);
		MessageBoxA(window, "wglMakeCurrent failed", "ERROR", MB_OK);
		return NULL;
	}

	return hglrc;
}

void shutdown_opengl2(HWND window, HGLRC hglrc)
{
	HDC hdc = GetDC(window);
	if (!hdc)
	{
		MessageBoxA(window, "GetDC failed", "ERROR", MB_OK);
		return;
	}

	wglMakeCurrent(hdc, NULL);
	wglDeleteContext(hglrc);
}

int CALLBACK wWinMain(HINSTANCE /*inst*/, HINSTANCE /*prev*/, LPWSTR /*cmd*/, int /*show*/)
{
	WNDCLASSEXW wc = {0};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc = borderless_window_proc;
	wc.hInstance = HINST_THISCOMPONENT;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = L"borderless-window";
	ATOM cls = RegisterClassExW(&wc);

	struct window *data = (struct window*)calloc(1, sizeof(struct window));
	data->hwnd = CreateWindowExW(
		WS_EX_APPWINDOW | WS_EX_LAYERED,
		(LPWSTR)MAKEINTATOM(cls),
		L"Borderless Window",
		WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 1280, 960,
		NULL, NULL, HINST_THISCOMPONENT, data);

	/* Make the window a layered window so the legacy GDI API can be used to
	   draw to it without messing up the area on top of the DWM frame. Note:
	   This is not necessary if other drawing APIs are used, eg. GDI+, OpenGL,
	   Direct2D, Direct3D, DirectComposition, etc. */
	
	// Apparently it actually is necessary if you want compositing to work properly with alpha blending!?
	SetLayeredWindowAttributes(data->hwnd, RGB(255, 255, 255), 255, LWA_COLORKEY);

	handle_compositionchanged(data);
	handle_themechanged(data);

	HGLRC hglrc = setup_opengl2(data->hwnd);
	MSG message = {0};
	if (hglrc)
	{
		handle_init(data);

		ShowWindow(data->hwnd, SW_SHOWDEFAULT);
		UpdateWindow(data->hwnd);

		while (GetMessageW(&message, NULL, 0, 0)) {
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}

		handle_shutdown(data);
		shutdown_opengl2(data->hwnd, hglrc);
	}

	free(data);
	UnregisterClassW((LPWSTR)MAKEINTATOM(cls), HINST_THISCOMPONENT);
	return (int)message.wParam;
}
