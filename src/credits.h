/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef CREDITS_H
#define CREDITS_H

void credits_input(void);
void credits_loop(void);
void credits_add(char*, int);

#define CREDITS_ENTRY_FADEIN 200.0
#define CREDITS_ENTRY_FADEOUT 100.0
#define CREDITS_YUKKURI_SCALE 0.5

#endif
