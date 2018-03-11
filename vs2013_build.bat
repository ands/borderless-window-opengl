@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
cd /d %~dp0
set INCLUDE_DIRS=-I. -Iimgui_winapi_gl2
set SOURCES=borderless-window.cpp borderless-window-rendering.cpp
set IMGUI_SOURCES=imgui_winapi_gl2/imgui.cpp imgui_winapi_gl2/imgui_demo.cpp imgui_winapi_gl2/imgui_draw.cpp imgui_winapi_gl2/imgui_impl_gl2.cpp
rc resources.rc
cl -nologo -MTd -Od -Oi -fp:fast -Gm- -EHsc -GR- -WX -W4 -FC -Z7 %INCLUDE_DIRS% %SOURCES% %IMGUI_SOURCES% /link resources.res -incremental:no -opt:ref
pause