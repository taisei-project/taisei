/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage4_events.h"
#include <global.h>

void stage4_events() {
	TIMER(&global.timer);
	
	FROM_TO(0, 500, 10)
		create_projectile2c("bullet", VIEWPORT_W/2, rgb(0.3,0,1), asymptotic, 2I+1-2*frand(), 5);
}