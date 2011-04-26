/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef LIST_H
#define LIST_H

/* I got annoyed of the code doubling caused by simple linked lists,
 * so i do some void-magic here to save the lines.
 */

void *create_element(void **dest, int size);
void delete_element(void **dest, void *e);
void *delete_all_elements(void **dest, void (callback)(void **, void *));

#endif