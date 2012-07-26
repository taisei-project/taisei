/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage4_events.h"
#include <global.h>

complex laccel(Laser *l, float t) {
	return l->pos + l->args[0]*t + 0.5*l->args[1]*t*t;
}

void stage4_events() {
	TIMER(&global.timer);
	
	FROM_TO(0, 500, 200)
// 		create_projectile2c("bullet", VIEWPORT_W/2, rgb(0.3,0,1), asymptotic, 2I+1-2*frand(), 5);
		create_lasercurve1c(VIEWPORT_W/2 + VIEWPORT_H/2*I, 20, 50, rgb(0.3,1,1), las_linear, 3*cexp(2I*M_PI*frand()));
}