/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"
#include "renderer/api.h"

DEFINE_RESOURCE_GETTER(Model, res_model, RES_MODEL)
DEFINE_OPTIONAL_RESOURCE_GETTER(Model, res_model_optional, RES_MODEL)

extern ResourceHandler model_res_handler;
