/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "resource.h"
#include "hashtable.h"

typedef struct I18nLocale I18nLocale;
extern ResourceHandler locale_res_handler;

const char *i18n_locale_get_translation_prehashed(I18nLocale *locale, const char *source, hash_t source_hash);

INLINE const char *i18n_locale_get_translation(I18nLocale *locale, const char *source) {
	return i18n_locale_get_translation_prehashed(locale, source, htutil_hashfunc_string(source));
}

char **i18n_find_locales(size_t *num_results);   // Free result with vfs_dir_list_free()

DEFINE_OPTIONAL_RESOURCE_GETTER(I18nLocale, res_locale, RES_LOCALE)
