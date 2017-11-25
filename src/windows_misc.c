/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include <windows.h>

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
