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
#include "borderless-window.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00AE)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME (0x00AF)
#endif

static void update_region(borderless_window_t *window)
{
	RECT old_rgn = window->rgn;
	RECT r = {0};

	if (IsMaximized(window->hwnd)) {
		WINDOWINFO wi = {};
		wi.cbSize = sizeof(wi);
		GetWindowInfo(window->hwnd, &wi);

		/* For maximized windows, a region is needed to cut off the non-client
		   borders that hang over the edge of the screen */
		window->rgn.left = wi.rcClient.left - wi.rcWindow.left;
		window->rgn.top = wi.rcClient.top - wi.rcWindow.top;
		window->rgn.right = wi.rcClient.right - wi.rcWindow.left;
		window->rgn.bottom = wi.rcClient.bottom - wi.rcWindow.top;
	} else if (!window->composition_enabled) {
		/* For ordinary themed windows when composition is disabled, a region
		   is needed to remove the rounded top corners. Make it as large as
		   possible to avoid having to change it when the window is resized. */
		window->rgn.left = 0;
		window->rgn.top = 0;
		window->rgn.right = 32767;
		window->rgn.bottom = 32767;
	} else {
		/* Don't mess with the region when composition is enabled and the
		   window is not maximized, otherwise it will lose its shadow */
		window->rgn = r;
	}

	/* Avoid unnecessarily updating the region to avoid unnecessary redraws */
	if (EqualRect(&window->rgn, &old_rgn))
		return;
	/* Treat empty regions as NULL regions */
	if (EqualRect(&window->rgn, &r))
		SetWindowRgn(window->hwnd, NULL, TRUE);
	else
		SetWindowRgn(window->hwnd, CreateRectRgnIndirect(&window->rgn), TRUE);
}

static void handle_nccreate(HWND hwnd, CREATESTRUCTW *cs)
{
	borderless_window_t *window = (borderless_window_t*)cs->lpCreateParams;
	SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)window);
}

static void handle_compositionchanged(borderless_window_t *window)
{
	BOOL enabled = FALSE;
	DwmIsCompositionEnabled(&enabled);
	window->composition_enabled = enabled == TRUE;

	if (enabled) {
		/* The window needs a frame to show a shadow, so give it the smallest
		   amount of frame possible */
		MARGINS m = {0};
		m.cyTopHeight = 1;
		DwmExtendFrameIntoClientArea(window->hwnd, &m);
		DWORD state = DWMNCRP_ENABLED;
		DwmSetWindowAttribute(window->hwnd, DWMWA_NCRENDERING_POLICY, &state, sizeof(DWORD));
	}

	update_region(window);
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

static void handle_nccalcsize(borderless_window_t *window, WPARAM wparam,
                              LPARAM lparam)
{
	union {
		LPARAM lparam;
		RECT* rect;
	} params = { lparam };

	/* DefWindowProc must be called in both the maximized and non-maximized
	   cases, otherwise tile/cascade windows won't work */
	RECT nonclient = *params.rect;
	DefWindowProcW(window->hwnd, WM_NCCALCSIZE, wparam, params.lparam);
	RECT client = *params.rect;

	if (IsMaximized(window->hwnd)) {
		WINDOWINFO wi = {0};
		wi.cbSize = sizeof(wi);
		GetWindowInfo(window->hwnd, &wi);

		/* Maximized windows always have a non-client border that hangs over
		   the edge of the screen, so the size proposed by WM_NCCALCSIZE is
		   fine. Just adjust the top border to remove the window title. */
		RECT r;
		r.left = client.left;
		r.top = nonclient.top + wi.cyWindowBorders;
		r.right = client.right;
		r.bottom = client.bottom;
		*params.rect = r;

		HMONITOR mon = MonitorFromWindow(window->hwnd, MONITOR_DEFAULTTOPRIMARY);
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

static LRESULT handle_nchittest(borderless_window_t *window, int x, int y)
{
	if (IsMaximized(window->hwnd))
		return HTCLIENT;

	POINT mouse = { x, y };
	ScreenToClient(window->hwnd, &mouse);

	/* The horizontal frame should be the same size as the vertical frame,
	   since the NONCLIENTMETRICS structure does not distinguish between them */
	int frame_size = GetSystemMetrics(SM_CXFRAME); // + GetSystemMetrics(SM_CXPADDEDBORDER);
	/* The diagonal size handles are wider than the frame */
	int diagonal_width = frame_size * 2 + GetSystemMetrics(SM_CXBORDER);

	if (mouse.y < frame_size) {
		if (mouse.x < diagonal_width)
			return HTTOPLEFT;
		if (mouse.x >= (int)window->width - diagonal_width)
			return HTTOPRIGHT;
		return HTTOP;
	}

	if (mouse.y >= (int)window->height - frame_size) {
		if (mouse.x < diagonal_width)
			return HTBOTTOMLEFT;
		if (mouse.x >= (int)window->width - (int)diagonal_width)
			return HTBOTTOMRIGHT;
		return HTBOTTOM;
	}

	if (mouse.x < frame_size)
		return HTLEFT;
	if (mouse.x >= (int)window->width - frame_size)
		return HTRIGHT;
	return HTCLIENT;
}

static void handle_themechanged(borderless_window_t *window)
{
	window->theme_enabled = IsThemeActive() == TRUE;
}

static void handle_windowposchanged(borderless_window_t *window, const WINDOWPOS *pos)
{
	RECT client;
	GetClientRect(window->hwnd, &client);
	unsigned old_width = window->width;
	unsigned old_height = window->height;
	window->width = client.right;
	window->height = client.bottom;
	bool client_changed = window->width != old_width || window->height != old_height;

	if (client_changed || (pos->flags & SWP_FRAMECHANGED))
		update_region(window);

	if (client_changed) {
		/* Invalidate the changed parts of the rectangle drawn in WM_PAINT */
		if (window->width > old_width) {
			RECT r = { (LONG)(old_width - 1), (LONG)0, (LONG)old_width, (LONG)old_height };
			InvalidateRect(window->hwnd, &r, TRUE);
		} else {
			RECT r = { (LONG)(window->width - 1), (LONG)0, (LONG)window->width, (LONG)window->height };
			InvalidateRect(window->hwnd, &r, TRUE);
		}
		if (window->height > old_height) {
			RECT r = { (LONG)0, (LONG)(old_height - 1), (LONG)old_width, (LONG)old_height };
			InvalidateRect(window->hwnd, &r, TRUE);
		} else {
			RECT r = { (LONG)0, (LONG)(window->height - 1), (LONG)window->width, (LONG)window->height };
			InvalidateRect(window->hwnd, &r, TRUE);
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

static LRESULT CALLBACK borderless_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	borderless_window_t *window = (borderless_window_t*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (!window) {
		/* Due to a longstanding Windows bug, overlapped windows will receive a
		   WM_GETMINMAXINFO message before WM_NCCREATE. This is safe to ignore.
		   It doesn't need any special handling anyway. */
		if (msg == WM_NCCREATE)
			handle_nccreate(hwnd, (CREATESTRUCTW*)lparam);
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}

	static bool painting = false;

	switch (msg) {
	case WM_DWMCOMPOSITIONCHANGED:
		handle_compositionchanged(window);
		return 0;
	case WM_NCACTIVATE:
		/* DefWindowProc won't repaint the window border if lParam (normally a HRGN) is -1. This is recommended in:
		   https://blogs.msdn.microsoft.com/wpfsdk/2008/09/08/custom-window-chrome-in-wpf/ */
		return DefWindowProcW(hwnd, msg, wparam, -1);
	case WM_NCCALCSIZE:
		handle_nccalcsize(window, wparam, lparam);
		return 0;
	case WM_NCHITTEST:
		return handle_nchittest(window, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
	case WM_NCPAINT:
		/* Only block WM_NCPAINT when composition is disabled. If it's blocked
		   when composition is enabled, the window shadow won't be drawn. */
		if (!window->composition_enabled)
			return 0;
		break;
	case WM_NCUAHDRAWCAPTION:
	case WM_NCUAHDRAWFRAME:
		/* These undocumented messages are sent to draw themed window borders.
		   Block them to prevent drawing borders over the client area. */
		return 0;
	case WM_PAINT:
		if (painting)
			return DefWindowProcW(hwnd, msg, wparam, lparam);
		painting = true;
		break;
	case WM_SETICON:
	case WM_SETTEXT:
		/* Disable painting while these messages are handled to prevent them
		   from drawing a window caption over the client area, but only when
		   composition and theming are disabled. These messages don't paint
		   when composition is enabled and blocking WM_NCUAHDRAWCAPTION should
		   be enough to prevent painting when theming is enabled. */
		if (!window->composition_enabled && !window->theme_enabled)
			return handle_message_invisible(hwnd, msg, wparam, lparam);
		break;
	case WM_SIZE:
		window->minimized = wparam == SIZE_MINIMIZED;
		window->maximized = wparam == SIZE_MAXIMIZED;
		return 0;
	case WM_THEMECHANGED:
		handle_themechanged(window);
		break;
	case WM_WINDOWPOSCHANGED:
		handle_windowposchanged(window, (WINDOWPOS*)lparam);
		return DefWindowProcW(hwnd, msg, wparam, lparam); // Must be executed so that WM_SIZE and WM_MOVE get sent properly!
	}

	LRESULT result = window->handler(window, msg, wparam, lparam) ? 0 : DefWindowProcW(hwnd, msg, wparam, lparam);

	if (msg == WM_PAINT)
		painting = false;
	else if (msg == WM_CLOSE || msg == WM_QUIT)
	{
		DestroyWindow(hwnd);
		free(window);
		return 0;
	}

	return result;
}

void borderless_window_register()
{
	WNDCLASSEXW wc = {0};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc = borderless_window_proc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszClassName = L"borderless-window";
	RegisterClassExW(&wc);
}

void borderless_window_unregister()
{
	UnregisterClassW(L"borderless-window", GetModuleHandle(NULL));
}

borderless_window_t* borderless_window_create(LPCWSTR title, int width, int height, message_handler handler, void* userdata)
{
	borderless_window_t *window = (borderless_window_t*)calloc(1, sizeof(borderless_window_t));

	window->handler = handler;
	window->userdata = userdata;

	window->hwnd = CreateWindowExW(
		WS_EX_APPWINDOW | WS_EX_LAYERED,
		L"borderless-window",
		title,
		WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		NULL, NULL, GetModuleHandle(NULL), window);
	
	window->hdc = GetDC(window->hwnd);

	/* Make the window a layered window so the legacy GDI API can be used to
	   draw to it without messing up the area on top of the DWM frame. Note:
	   This is not necessary if other drawing APIs are used, eg. GDI+, OpenGL,
	   Direct2D, Direct3D, DirectComposition, etc. */
	
	// Apparently it actually is necessary if you want compositing to work properly with alpha blending!?
	SetLayeredWindowAttributes(window->hwnd, RGB(255, 255, 255), 255, LWA_COLORKEY);

	handle_compositionchanged(window);
	handle_themechanged(window);

	return window;
}

static BOOL CALLBACK enum_thread_window(HWND hwnd, LPARAM /*lParam*/)
{
	PostMessage(hwnd, WM_CLOSE, 0, 0);
	return TRUE;
}

void borderless_window_close_all(borderless_window_t* root)
{
	EnumThreadWindows(GetWindowThreadProcessId(root->hwnd, 0), enum_thread_window, NULL);
}
