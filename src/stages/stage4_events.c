/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage4_events.h"
#include <global.h>

complex laccel(Laser *l, float t) {
	return l->pos + l->args[0]*t + 100I*sin(t*0.1);
}

void stage4_events() {
	TIMER(&global.timer);
	
	FROM_TO(0, 500, 500)
// 		create_projectile2c("bullet", VIEWPORT_W/2, rgb(0.3,0,1), asymptotic, 2I+1-2*frand(), 5);
		create_lasercurve2c(VIEWPORT_W/2+VIEWPORT_H/2*I, 100, 200, rgb(0,1,1), laccel, -2 -2I, 0.02I);
}