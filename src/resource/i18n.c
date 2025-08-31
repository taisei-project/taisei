/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "intl/intl.h"
#include "resource.h"
#include "util/io.h"

#include "i18n.h"

#define I18N_PATH_SUFFIX "/LC_MESSAGES/taisei.mo"

static char *i18n_path(const char *name) {
	return try_path(I18N_PATH_PREFIX, name, I18N_PATH_SUFFIX);
}

static bool i18n_check_path(const char *path) {
	return strstartswith(path, I18N_PATH_PREFIX) && strendswith(path, I18N_PATH_SUFFIX);
}

static void i18n_load(ResourceLoadState *st) {
	IntlTextDomain *domain = intl_read_mo(st->path);

	if(!domain) {
		res_load_failed(st);
		return;
	}

	res_load_finished(st, domain);
}

static void i18n_unload(void *domain) {
	intl_textdomain_destroy(domain);
}

ResourceHandler i18n_res_handler = {
	.type = RES_I18N,
	.typename = "i18n",
	.subdir = I18N_PATH_PREFIX,

	.procs = {
		.find = i18n_path,
		.check = i18n_check_path,
		.load = i18n_load,
		.unload = i18n_unload,
	},
};

void i18n_init(ResourceGroup *rg) {
	res_group_preload(rg, RES_I18N, RESF_DEFAULT,
		"zh_CN",
	NULL);
	intl_set_textdomain(res_i18n("zh_CN"));
}
