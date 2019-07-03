/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_script_console_h
#define IGUARD_script_console_h

#include "taisei.h"

void con_init(void);
void con_postinit(void);
void con_shutdown(void);
void con_update(void);
void con_draw(void);
void con_set_active(bool active);
bool con_is_active(void);
void con_print(const char *text) attr_nonnull(1);
void con_printf(const char *fmt, ...) attr_printf(1, 2) attr_nonnull(1);
void con_set_prompt(const char *prompt) attr_nonnull(1);

#endif // IGUARD_script_console_h
