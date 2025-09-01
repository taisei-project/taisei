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
	mem_free(domain);
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

static int fuzzy_match_system_locale(int n, const char *languages[n]) {
	int count;
	SDL_Locale **locales = SDL_GetPreferredLocales(&count);

	int chosen = -1;
	for(int i = 0; i < count; i++) {
		char *locale_str = strfmt("%s_%s", locales[i]->language, locales[i]->country);

		for(int j = 0; j < n; j++) {
			// complete match: good to go
			if(strcmp(languages[j], locale_str) == 0) {
				chosen = j;
				break;
			}

			// only languages match: ok but move on
			if(strncmp(languages[j], locales[i]->language, strlen(locales[i]->language)) == 0) {
				chosen = j;
			}
		}
		mem_free(locale_str);
	}

	SDL_free(locales);
	return chosen;
}

void i18n_set_textdomain_from_string(const char *name) {
	const char *languages[] = TAISEI_BUILDCONF_LANGUAGES;
	int chosen = -1;

	if(strcmp(name, "system") == 0) {
		chosen = fuzzy_match_system_locale(ARRAY_SIZE(languages), languages);
	} else if(strcmp(name, "en") == 0) {
		chosen = -1; // use builtin strings
	} else {
		for(int i = 0; i < ARRAY_SIZE(languages); i++) {
			if(strcmp(name, languages[i]) == 0) {
				chosen = i;
				break;
			}
		}
		if(chosen < 0) {
			log_error("No translation for %s available.", name);
		}
	}

	bool changed;
	if(chosen >= 0) {
		changed = intl_set_textdomain(res_i18n(languages[chosen]));
	} else {
		changed = intl_set_textdomain(NULL);
	}
}

void i18n_init(ResourceGroup *rg) {
	const char *languages[] = TAISEI_BUILDCONF_LANGUAGES;
	for(int i = 0; i < ARRAY_SIZE(languages); i++) {
		res_group_preload(rg, RES_I18N, RESF_DEFAULT,
			languages[i],
		NULL);
	}

	i18n_set_textdomain_from_string(config_get_str(CONFIG_LANGUAGE));
}
