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
#pragma GCC diagnostic ignored "-Wpedantic"

#define NX_LOG_FMT(fmt, ...) printf("[NX] " fmt "\n", ##__VA_ARGS__)
#define NX_LOG(str) NX_LOG_FMT("%s", str)
#define NX_SETENV(name, val) NX_LOG_FMT("Setting env var %s to %s", name, val);setenv(name, val, 1)

attr_used
void userAppInit() {
	socketInitializeDefault();

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
	socketExit();
}

#pragma GCC diagnostic pop
