#pragma once

struct window
{
	HWND hwnd;

	unsigned width;
	unsigned height;

	RECT rgn;

	bool theme_enabled;
	bool composition_enabled;
};

void handle_init(struct window *data);
bool handle_message(struct window *data, UINT msg, WPARAM wparam, LPARAM lparam);
void handle_shutdown(struct window *data);
