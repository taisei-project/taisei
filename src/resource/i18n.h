/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "intl/intl.h"
#include "taisei.h"

#include "resource.h"

extern ResourceHandler i18n_res_handler;

#define I18N_PATH_PREFIX "res/i18n/"

void i18n_init(void);

DEFINE_OPTIONAL_RESOURCE_GETTER(IntlTextDomain, res_i18n, RES_I18N)
