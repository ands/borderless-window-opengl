#pragma once

struct borderless_window_t;

typedef void (*imgui_window_func)(borderless_window_t *window, void *userdata);

void imgui_window_init(int openglMajorVersion, int openglMinorVersion);
borderless_window_t * imgui_window_create(LPCWSTR title, int w, int h, imgui_window_func draw, imgui_window_func close, void *userdata);
int imgui_window_message_loop();
void imgui_window_shutdown();

bool imgui_window_begin(const char *title);
void imgui_window_end();
