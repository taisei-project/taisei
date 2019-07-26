/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2019, p-sam <p-sam@d3vs.net>.
 */

#include "arch_switch.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <switch/runtime/devices/socket.h>
#include <switch/runtime/nxlink.h>
#include <switch/services/applet.h>
#include <switch/services/fs.h>

#include "taisei.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wformat-truncation"

#define NX_LOG_FMT(fmt, ...) printf("[NX] " fmt "\n", ##__VA_ARGS__)
#define NX_LOG(str) NX_LOG_FMT("%s", str)
#define NX_SETENV(name, val) NX_LOG_FMT("Setting env var %s to %s", name, val);setenv(name, val, 1)

static nxAtExitFn g_nxAtExitFn = NULL;
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
void userAppInit() {
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

	char cwd[FS_MAX_PATH] = {0};
	getcwd(cwd, FS_MAX_PATH);

	char path[FS_MAX_PATH] = {0};
	snprintf(path, FS_MAX_PATH, "%s/%s", cwd, TAISEI_BUILDCONF_DATA_PATH);
	NX_SETENV("TAISEI_RES_PATH", path);
	snprintf(path, FS_MAX_PATH, "%s/storage", cwd);
	NX_SETENV("TAISEI_STORAGE_PATH", path);
	snprintf(path, FS_MAX_PATH, "%s/cache", cwd);
	NX_SETENV("TAISEI_CACHE_PATH", path);

#if defined(DEBUG) && defined(TAISEI_BUILDCONF_DEBUG_OPENGL)
	// enable Mesa logging:
	NX_SETENV("EGL_LOG_LEVEL", "debug");
	NX_SETENV("MESA_VERBOSE", "all");
	NX_SETENV("MESA_DEBUG", "1");
	NX_SETENV("NOUVEAU_MESA_DEBUG", "1");
	snprintf(path, FS_MAX_PATH, "%s/cache", cwd);

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
void userAppExit() {
	if(g_nxAtExitFn != NULL) {
		NX_LOG("calling exit callback");
		g_nxAtExitFn();
		g_nxAtExitFn = NULL;
	}
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

void __attribute__((weak)) __libnx_exit(int rc);

void nxExit(int rc) {
	__libnx_exit(rc);
}

#pragma GCC diagnostic pop
