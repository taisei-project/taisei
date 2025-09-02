/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "intl/intl.h"
#include "taisei.h"

#include "resource.h"

extern ResourceHandler i18n_res_handler;

#define I18N_PATH_PREFIX "res/i18n/"

void i18n_init(void);
void i18n_set_textdomain_from_string(const char *name);

DEFINE_OPTIONAL_RESOURCE_GETTER(IntlTextDomain, res_i18n, RES_I18N)
