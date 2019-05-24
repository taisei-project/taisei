/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_script_script_h
#define IGUARD_script_script_h

#include "taisei.h"

typedef enum ScriptReplResult {
	REPL_OK,
	REPL_NEEDMORE,
	REPL_ERROR,
} ScriptReplResult;

void script_init(void);
void script_shutdown(void);
ScriptReplResult script_repl(const char *chunk);

#endif // IGUARD_script_script_h
