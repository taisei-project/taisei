/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "i18n.h"

#include "config.h"
#include "log.h"
#include "resource/locale.h"
#include "util.h"
#include "util/env.h"

#ifdef DEBUG
	#define I18N_CHECK_MISSING_DEFAULT 1
#else
	#define I18N_CHECK_MISSING_DEFAULT 0
#endif

static struct i18n {
	ResourceGroup resources;
	I18nLocale *active_locale;
	int active_locale_idx;
	char **known_locales;
	size_t num_known_locales;
	ht_strset_ts_t missed_strings;
} i18n;

static bool i18n_misschecker_active(void) {
	return i18n.missed_strings.elements != NULL;
}

static void i18n_misschecker_init(bool reset_only) {
	if(!env_get("TAISEI_I18N_CHECK_MISSING", I18N_CHECK_MISSING_DEFAULT)) {
		return;
	}

	if(!i18n_misschecker_active() && !reset_only) {
		ht_create(&i18n.missed_strings);
	} else {
		ht_unset_all(&i18n.missed_strings);
	}
}

void i18n_init(void) {
	res_group_init(&i18n.resources);
	i18n.known_locales = i18n_find_locales(&i18n.num_known_locales);

	StringBuffer buf = {};
	for(size_t i = 0; i < i18n.num_known_locales; ++i) {
		if(i) {
			strbuf_cat(&buf, ", ");
		}

		strbuf_cat(&buf, i18n.known_locales[i]);
	}

	if(buf.start) {
		log_info("Found %zu locales: %s", i18n.num_known_locales, buf.start);
	} else {
		log_info("No locales found");
	}

	strbuf_free(&buf);

	i18n.active_locale_idx = -1;
	i18n_set_locale(config_get_str(CONFIG_LOCALE));
}

void i18n_shutdown(void) {
	res_group_release(&i18n.resources);
	vfs_dir_list_free(i18n.known_locales, i18n.num_known_locales);
	ht_destroy(&i18n.missed_strings);
	i18n = (typeof(i18n)) {};
}

static int fuzzy_match_locales(
	size_t num_preferred_locales, const SDL_Locale *preferred_locales[num_preferred_locales]
) {
	int chosen = -1;

	for(int i = 0; i < num_preferred_locales; i++) {
		const SDL_Locale *preferred_locale = preferred_locales[i];
		char locale_str[32];

		size_t language_len = strlen(preferred_locale->language);

		if(preferred_locale->country) {
			snprintf(locale_str, sizeof(locale_str), "%s_%s", preferred_locale->language, preferred_locale->country);
		} else {
			memcpy(locale_str, preferred_locale->language, language_len);
		}

		for(size_t j = 0; j < i18n.num_known_locales; j++) {
			// complete match: good to go
			if(strcasecmp(i18n.known_locales[j], locale_str) == 0) {
				return j;
			}

			// only languages match: ok but move on
			if(strncasecmp(i18n.known_locales[j], preferred_locale->language, language_len) == 0) {
				chosen = j;
			}
		}

		if(chosen != -1) {
			// if we found a partial match, stop here before looking at lower-priority languages
			break;
		}
	}

	return chosen;
}

void i18n_set_locale(const char *locale_id) {
	int chosen = -1;

	if(!strcasecmp(locale_id, I18N_LOCALEID_SYSTEM)) {
		int num_locales;
		SDL_Locale **locales = SDL_GetPreferredLocales(&num_locales);

		if(locales) {
			chosen = fuzzy_match_locales(num_locales, (const SDL_Locale**)locales);
		} else {
			log_sdl_error(LOG_ERROR, "SDL_GetPreferredLocales");
		}

		SDL_free(locales);
	} else if(strcasecmp(locale_id, I18N_LOCALEID_BUILTIN)) {
		for(size_t j = 0; j < i18n.num_known_locales; j++) {
			if(strcasecmp(i18n.known_locales[j], locale_id) == 0) {
				chosen = j;
				break;
			}
		}
	}

	if(i18n.active_locale_idx != chosen) {
		res_group_release(&i18n.resources);
		i18n_misschecker_init(chosen < 0);

		if(chosen > -1) {
			assume(chosen < i18n.num_known_locales);
			const char *id = i18n.known_locales[chosen];
			res_group_preload(&i18n.resources, RES_LOCALE, RESF_OPTIONAL, id, NULL);
			i18n.active_locale = res_locale(id);

			if(!i18n.active_locale) {
				chosen = -1;
			}
		} else {
			i18n.active_locale = NULL;
		}

		log_info("Active locale is now %s", chosen > -1 ? i18n.known_locales[chosen] : I18N_LOCALEID_BUILTIN);

		i18n.active_locale_idx = chosen;
	}
}

const char *const *i18n_list_locales(size_t *num_locales) {
	*num_locales = i18n.num_known_locales;
	return (const char *const *)i18n.known_locales;
}

const char *_i18n_translate_prehashed(const char *msgid, hash_t hash) {
	if(!i18n.active_locale) {
		return msgid;
	}

	const char *result = i18n_locale_get_translation_prehashed(i18n.active_locale, msgid, hash);

	if(i18n_misschecker_active() && result == msgid) {
		if(ht_try_set_prehashed(&i18n.missed_strings, msgid, hash, HT_EMPTY, NULL, NULL)) {
			log_warn("Missing translation for string: %s", msgid);
		}
	}

	return result;
}
