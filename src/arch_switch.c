/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2019, p-sam <p-sam@d3vs.net>.
 */

#include "taisei.h"

#include "arch_switch.h"

#include <switch/services/applet.h>
#include <switch/services/fs.h>
#include <switch/services/ssl.h>
#include <switch/runtime/devices/socket.h>
#include <switch/runtime/nxlink.h>

#define NX_LOG_FMT(fmt, ...) tsfprintf(stdout, "[NX] " fmt "\n", ##__VA_ARGS__)
#define NX_LOG(str) NX_LOG_FMT("%s", str)
#define NX_SETENV(name, val) NX_LOG_FMT("Setting env var %s to %s", name, val);env_set_string(name, val, true)

uint32_t __nx_fs_num_sessions = 1;

static nxAtExitFn g_nxAtExitFn = NULL;
static char g_programDir[FS_MAX_PATH] = {0};
static AppletHookCookie g_hookCookie;

static void onAppletHook(AppletHookType hook, void *param) {
	switch (hook) {
		case AppletHookType_OnExitRequest:
			NX_LOG("Got AppletHook OnExitRequest, exiting.\n");
			taisei_quit();
			break;

		default:
			break;
	}
}

attr_used
void userAppInit(void) {
	socketInitializeDefault();
	appletLockExit();
	appletHook(&g_hookCookie, onAppletHook, NULL);

#ifdef DEBUG
	dup2(1, 2);
	NX_LOG("stderr -> stdout");
	nxlinkStdio();
	NX_LOG("nxlink enabled");
	NX_SETENV("TAISEI_NOASYNC", "1");
#endif

	appletInitializeGamePlayRecording();
	appletSetGamePlayRecordingState(1);

	getcwd(g_programDir, FS_MAX_PATH);

#if defined(DEBUG) && defined(TAISEI_BUILDCONF_DEBUG_OPENGL)
	// enable Mesa logging:
	NX_SETENV("EGL_LOG_LEVEL", "debug");
	NX_SETENV("MESA_VERBOSE", "all");
	NX_SETENV("MESA_DEBUG", "1");
	NX_SETENV("MESA_INFO", "1");
	NX_SETENV("MESA_GLSL", "errors");
	NX_SETENV("NOUVEAU_MESA_DEBUG", "1");
	NX_SETENV("LIBGL_DEBUG", "verbose");

	// enable shader debugging in Nouveau:
	NX_SETENV("NV50_PROG_OPTIMIZE", "0");
	NX_SETENV("NV50_PROG_DEBUG", "1");
	NX_SETENV("NV50_PROG_CHIPSET", "0x120");
#else
	// disable error checking and save CPU time
	NX_SETENV("MESA_NO_ERROR", "1");
#endif
}

attr_used
void userAppExit(void) {
	socketExit();
	appletUnlockExit();
}

int nxAtExit(nxAtExitFn fn) {
	if(g_nxAtExitFn == NULL) {
		NX_LOG("got exit callback");
		g_nxAtExitFn = fn;
		return 0;
	}
	return -1;
}

void __attribute__((weak)) noreturn __libnx_exit(int rc);

void noreturn nxExit(int rc) {
	if(g_nxAtExitFn != NULL) {
		NX_LOG("calling exit callback");
		g_nxAtExitFn();
		g_nxAtExitFn = NULL;
	}

	__libnx_exit(rc);
}

void noreturn nxAbort(void) {
	/* Using abort would not give us correct offsets in crash reports,
	 * nor code region name, so we use __builtin_trap instead */
	__builtin_trap();
}

const char* nxGetProgramDir(void) {
	return g_programDir;
}
