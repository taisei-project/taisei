/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "hashtable.h"

#define I18N_LOCALEID_SYSTEM  "system"
#define I18N_LOCALEID_BUILTIN "en"
#define I18N_LOCALENAME_SYSTEM  N_("System")
#define I18N_LOCALENAME_BUILTIN "English"

void i18n_init(void);
void i18n_shutdown(void);
void i18n_set_locale(const char *locale_id);
const char *const *i18n_list_locales(size_t *num_locales);
const char *i18n_get_locale_name(const char *locale_id);

const char *_i18n_translate_prehashed(const char *msgid, hash_t hash);

attr_format_arg(1) INLINE const char *i18n_translate(const char *msgid) {
	return _i18n_translate_prehashed(msgid, htutil_hashfunc_string(msgid));
}

#define gettext(msgid) i18n_translate(msgid)
#define pgettext(msgctxt, msgid) gettext(msgctxt "\004" msgid)

#define _(str) gettext(str)
// N_(str) marks strings for xgettext without translating them
#define N_(str) str
