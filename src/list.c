/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA.
 
 ---
 Copyright (C) 2010, Lukas Weber <laochailan@web.de>
 */

#include "list.h"

#include <stdlib.h>

typedef struct {
	void *next;
	void *prev;
} List;

void *create_element(void **dest, int size) {
	void *last = *dest;
	void *e = malloc(size);
	
	if(last != NULL) {
		while(((List *)last)->next != NULL)
			last = ((List *)last)->next;
		((List *)last)->next = e;
	} else {
		*dest = e;
	}
	
	((List *)e)->prev = last;
	((List *)e)->next = NULL;
	
	return e;
}

void *delete_element(void **dest, void *e) {
	if(((List *)e)->prev != NULL)
		((List *)((List *)e)->prev)->next = ((List *)e)->next;
	if(((List *)e)->next != NULL)
		((List *)((List *)e)->next)->prev = ((List *)e)->prev;	
	if(*dest == e)
		*dest = ((List *)e)->next;
	
	free(e);
}

void *delete_all_elements(void **dest) {
	void *e = *dest;
	void *tmp;
	
	while(e != 0) {
		tmp = e;
		e = ((List *)e)->next;
		delete_element(dest, tmp);
	} 
	
	*dest = NULL;
}
