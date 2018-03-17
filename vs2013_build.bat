@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
cd /d %~dp0
set INCLUDE_DIRS=-I. -Iimgui_winapi_gl3
set SOURCES=borderless-window.cpp borderless-window-rendering.cpp glad.c opengl_context.cpp
set IMGUI_SOURCES=imgui_winapi_gl3/imgui.cpp imgui_winapi_gl3/imgui_demo.cpp imgui_winapi_gl3/imgui_draw.cpp imgui_winapi_gl3/imgui_impl_gl3.cpp
set DISABLED_WARNINGS=-wd4054 -wd4055 -wd4706 -wd4127
cl -nologo -MTd -Od -Oi -fp:fast -Gm- -EHsc -GR- -WX -W4 %DISABLED_WARNINGS% -FC -Z7 %INCLUDE_DIRS% %SOURCES% %IMGUI_SOURCES% /link -incremental:no -opt:ref
pause