#pragma once

struct borderless_window_t;

typedef bool (*message_handler)(borderless_window_t *data, unsigned int msg, unsigned __int64 wparam, unsigned __int64 lparam);

struct borderless_window_t
{
	void *hwnd;
	void *hdc;

	unsigned width;
	unsigned height;

	bool minimized;
	bool maximized;
	bool composition_enabled;
	bool painting;

	message_handler handler;
	void *userdata;
};

void borderless_window_register();
void borderless_window_unregister();

borderless_window_t *borderless_window_create(const wchar_t *title, int width, int height, message_handler handler, void *userdata);
void borderless_window_close_all(borderless_window_t *root);
void borderless_window_close(borderless_window_t *window);
void borderless_window_quit();