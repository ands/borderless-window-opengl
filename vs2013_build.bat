@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
cd /d %~dp0
set INCLUDE_DIRS=-I. -Iimgui -Iimgui_borderless_window
set SOURCES=example.cpp
set WINDOW_SOURCES=imgui_borderless_window/borderless_window.cpp imgui_borderless_window/glad.c imgui_borderless_window/opengl_context.cpp imgui_borderless_window/imgui_window.cpp imgui_borderless_window/imgui_impl_gl3.cpp
set IMGUI_SOURCES=imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp
set DISABLED_WARNINGS=-wd4054 -wd4055 -wd4706 -wd4127
cl -nologo -MTd -Od -Oi -fp:fast -Gm- -EHsc -GR- -WX -W4 %DISABLED_WARNINGS% -FC -Z7 %INCLUDE_DIRS% %SOURCES% %WINDOW_SOURCES% %IMGUI_SOURCES% /link -incremental:no -opt:ref
pause