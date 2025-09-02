/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "systime.h"

#include <time.h>

void get_system_time(SystemTime *systime) {
	#if defined(TAISEI_BUILDCONF_HAVE_TIMESPEC)
		#if defined(TAISEI_BUILDCONF_HAVE_BUILTIN_AVAILABLE)
		if(__builtin_available(macOS 10.15, *)) {
			timespec_get(systime, TIME_UTC);
		} else {
			systime->tv_sec = time(NULL);
			systime->tv_nsec = 0;
		}
		#else
		timespec_get(systime, TIME_UTC);
		#endif
	#else
	systime->tv_sec = time(NULL);
	systime->tv_nsec = 0;
	#endif
}
