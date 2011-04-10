/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef SLAVE_H
#define SLAVE_H

#include "animation.h"
#include <complex.h>
#include <stdarg.h>

struct Slave;
typedef void (*SlaveRule)(struct Slave*);

typedef struct Slave {
	struct Slave *next;
	struct Slave *prev;
	
	long birthtime;
	
	complex pos;
	complex pos0;
	
	SlaveRule logic_rule;
	SlaveRule draw_rule;
	Animation *ani;
	
	void *parent;
	complex args[4];
} Slave;

void load_slave();

void create_slave(Slave **slaves, SlaveRule logic_rule, SlaveRule draw_rule, complex pos, void *parent, complex args, ...);
void delete_slave(Slave **slaves, Slave* slave);
void draw_slaves(Slave *slaves);
void free_slaves(Slave **slaves);

void process_slaves(Slave *slaves);

//void linear(complex *pos, complex pos0, float *angle, int time, complex* args);
#endif