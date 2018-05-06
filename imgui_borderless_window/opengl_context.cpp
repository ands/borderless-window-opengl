#include <windows.h>

#pragma comment(lib, "opengl32.lib")

typedef HGLRC WINAPI wglCreateContextAttribsARB_t(HDC hdc, HGLRC hShareContext, const int *attribList);
wglCreateContextAttribsARB_t *wglCreateContextAttribsARB;

// See https://www.opengl.org/registry/specs/ARB/wgl_create_context.txt for all values
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

typedef BOOL WINAPI wglChoosePixelFormatARB_t(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
wglChoosePixelFormatARB_t *wglChoosePixelFormatARB;

// See https://www.opengl.org/registry/specs/ARB/wgl_pixel_format.txt for all values
#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#define WGL_ACCELERATION_ARB                      0x2003
#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#define WGL_PIXEL_TYPE_ARB                        0x2013
#define WGL_COLOR_BITS_ARB                        0x2014
#define WGL_ALPHA_BITS_ARB                        0x201B
#define WGL_DEPTH_BITS_ARB                        0x2022
#define WGL_STENCIL_BITS_ARB                      0x2023

#define WGL_FULL_ACCELERATION_ARB                 0x2027
#define WGL_TYPE_RGBA_ARB                         0x202B

static void opengl_fatal_error(char *msg)
{
	MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONEXCLAMATION);
}

bool opengl_set_pixelformat(HDC hdc)
{
	int pixel_format_attribs[] = {
		WGL_DRAW_TO_WINDOW_ARB,     1,
		WGL_SUPPORT_OPENGL_ARB,     1,
		WGL_DOUBLE_BUFFER_ARB,      1,
		WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
		WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,         24,
		WGL_ALPHA_BITS_ARB,         8,
		WGL_DEPTH_BITS_ARB,         24,
		WGL_STENCIL_BITS_ARB,       8,
		0
	};

	int pixel_format;
	UINT num_formats;
	wglChoosePixelFormatARB(hdc, pixel_format_attribs, 0, 1, &pixel_format, &num_formats);
	if (!num_formats)
		return opengl_fatal_error("Failed to choose a modern OpenGL pixel format."), false;

	PIXELFORMATDESCRIPTOR pfd = {0};
	DescribePixelFormat(hdc, pixel_format, sizeof(pfd), &pfd);
	if (!SetPixelFormat(hdc, pixel_format, &pfd))
		return opengl_fatal_error("Failed to set the modern OpenGL pixel format."), false;

	return true;
}

HGLRC opengl_create_context(HDC hdc, int version_major, int version_minor)
{
	WNDCLASSA wc = {0};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = DefWindowProcA;
	wc.hInstance = GetModuleHandle(0);
	wc.lpszClassName = "Dummy_WGL";
	if (!RegisterClassA(&wc))
		return opengl_fatal_error("Failed to register dummy OpenGL window."), NULL;

	HWND dummy_window = CreateWindowExA(0, wc.lpszClassName, "Dummy OpenGL Window", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, wc.hInstance, 0);
	if (!dummy_window)
		return opengl_fatal_error("Failed to create dummy OpenGL window."), NULL;

	HDC dummy_dc = GetDC(dummy_window);

	PIXELFORMATDESCRIPTOR pfd = {0};
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	int pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
	if (!pixel_format)
		return opengl_fatal_error("Failed to find a suitable pixel format."), NULL;
	if (!SetPixelFormat(dummy_dc, pixel_format, &pfd))
		return opengl_fatal_error("Failed to set the pixel format."), NULL;

	HGLRC dummy_context = wglCreateContext(dummy_dc);
	if (!dummy_context)
		return opengl_fatal_error("Failed to create a dummy OpenGL rendering context."), NULL;
	if (!wglMakeCurrent(dummy_dc, dummy_context))
		return opengl_fatal_error("Failed to activate dummy OpenGL rendering context."), NULL;

	wglCreateContextAttribsARB = (wglCreateContextAttribsARB_t*)wglGetProcAddress("wglCreateContextAttribsARB");
	wglChoosePixelFormatARB = (wglChoosePixelFormatARB_t*)wglGetProcAddress("wglChoosePixelFormatARB");
	wglMakeCurrent(dummy_dc, 0);
	wglDeleteContext(dummy_context);
	ReleaseDC(dummy_window, dummy_dc);
	DestroyWindow(dummy_window);

	if (!opengl_set_pixelformat(hdc))
		return NULL;

	int gl_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, version_major,
		WGL_CONTEXT_MINOR_VERSION_ARB, version_minor,
		WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		0,
	};
	HGLRC gl_context = wglCreateContextAttribsARB(hdc, 0, gl_attribs);
	if (!gl_context)
		return opengl_fatal_error("Failed to create modern OpenGL context."), NULL;
	if (!wglMakeCurrent(hdc, gl_context))
		return opengl_fatal_error("Failed to activate modern OpenGL rendering context."), NULL;

	return gl_context;
}

void opengl_destroy_context(HGLRC hglrc)
{
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hglrc);
}
