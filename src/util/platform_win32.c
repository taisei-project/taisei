/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "systime.h"

static time_t win32time_to_posixtime(SYSTEMTIME *wtime) {
	struct tm ptime = {};

	ptime.tm_year = wtime->wYear - 1900;
	ptime.tm_mon = wtime->wMonth - 1;
	ptime.tm_mday = wtime->wDay;
	ptime.tm_hour = wtime->wHour;
	ptime.tm_min = wtime->wMinute;
	ptime.tm_sec = wtime->wSecond;
	ptime.tm_isdst = -1;

	return mktime(&ptime);
}

void get_system_time(SystemTime *systime) {
	SYSTEMTIME ltime;
	GetLocalTime(&ltime);

	systime->tv_sec = win32time_to_posixtime(&ltime);
	systime->tv_nsec = ltime.wMilliseconds * 1000000;
}

/*
 *  This is here for Windows laptops with hybrid graphics.
 *  We tell the driver to prefer the fast GPU over the integrated one by default.
 */

// Nvidia:
// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf

__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

// AMD:
// https://community.amd.com/thread/169965
// https://stackoverflow.com/questions/17458803/amd-equivalent-to-nvoptimusenablement

__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
