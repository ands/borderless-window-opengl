#pragma once

typedef bool (*message_handler)(struct window *data, UINT msg, WPARAM wparam, LPARAM lparam);

struct window
{
	HWND hwnd;
	HDC hdc;

	unsigned width;
	unsigned height;

	RECT rgn;

	bool minimized;
	bool maximized;
	bool theme_enabled;
	bool composition_enabled;

	message_handler handler;
	void* userdata;
};

void borderless_window_register();
void borderless_window_unregister();

struct window* borderless_window_create(LPCWSTR title, int width, int height, message_handler handler, void* userdata);
void borderless_window_destroy(struct window* data);
