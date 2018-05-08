#pragma once

#include "borderless_window.h"
typedef borderless_window_t imgui_window_t;

// Main/Messageloop API:
void imgui_window_init(int openglMajorVersion, int openglMinorVersion);
int imgui_window_message_loop();
void imgui_window_shutdown();

// Window API:
typedef void (*imgui_window_func)(imgui_window_t *window, void *userdata);
imgui_window_t * imgui_window_create(const wchar_t *title, int w, int h, imgui_window_func draw, imgui_window_func close, void *userdata);
void imgui_window_close_all(imgui_window_t *root);
void imgui_window_close(imgui_window_t *window);
void imgui_window_quit();

// IMGUI helper API:
bool imgui_window_begin(const char *title, ImGuiWindowFlags flags = 0);
void imgui_window_end();
