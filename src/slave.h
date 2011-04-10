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

struct Slave;
//typedef void (*SlaveRule)(complex *pos, complex pos0, float *angle, int time, complex* args);

typedef struct Slave {
	struct Slave *next;
	struct Slave *prev;
	
	long birthtime;
	
	complex pos;
	complex pos0;
	
	float angle;
	
	//SlaveRule rule;
	//Animation *ani;
	
	//complex args[4];
} Slave;

void load_slave();

void create_slave(complex pos, complex pos0);
void delete_slave(Slave *slave);
void draw_slaves();
void free_slaves();

void process_slaves();

//void linear(complex *pos, complex pos0, float *angle, int time, complex* args);
#endif