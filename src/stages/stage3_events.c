/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "stage3_events.h"
#include "global.h"
#include "stage.h"
#include "enemy.h"
#include "laser.h"

void stage3_events() {
	TIMER(&global.timer);
	
	FROM_TO(30, 200, 20) {
		create_laser(LT_Line, VIEWPORT_W/2, 20*cexp(I*0.3*_i), 40, 200, rgb(0.9,0.9,0.0), NULL, 0);
	}
	
}