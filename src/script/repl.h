/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_script_repl_h
#define IGUARD_script_repl_h

#include "taisei.h"

typedef enum ScriptReplResult {
	REPL_OK,
	REPL_NEEDMORE,
	REPL_ERROR,
} ScriptReplResult;

ScriptReplResult script_repl(const char *chunk);
void script_repl_abort(void);

#endif // IGUARD_script_repl_h
