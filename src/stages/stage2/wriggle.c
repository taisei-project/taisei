/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "wriggle.h"

#include "boss.h"
#include "i18n/i18n.h"

Boss *stage2_spawn_wriggle(cmplx pos) {
	Boss *wriggle = create_boss(_("Wriggle"), "wriggle", pos);
	wriggle->glowcolor = *RGB(0.2, 0.4, 0.5);
	wriggle->shadowcolor = *RGBA_MUL_ALPHA(0.4, 0.2, 0.6, 0.5);
	return wriggle;
}
