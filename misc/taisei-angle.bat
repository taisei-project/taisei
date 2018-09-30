@echo off

cd "%~dp0"
set ANGLE_PATH=.\ANGLE
set TAISEI_RENDERER=gles30
set SDL_OPENGL_ES_DRIVER=1
set SDL_VIDEO_GL_DRIVER=%ANGLE_PATH%\libGLESv2.dll
set SDL_VIDEO_EGL_DRIVER=%ANGLE_PATH%\libEGL.dll

.\taisei.exe %*
