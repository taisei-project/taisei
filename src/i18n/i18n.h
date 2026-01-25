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
#include "i18n/format_strings.h"

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

INLINE const char *i18n_translate(const char *msgid) {
	return _i18n_translate_prehashed(msgid, htutil_hashfunc_string(msgid));
}

attr_format_arg(1) INLINE const char *i18n_translate_format(const char *msgid) {
	return i18n_translate(msgid);
}

#define _(str) i18n_translate(str)
// N_(str) marks strings for xgettext without translating them
#define N_(str) str

// F_(str) is like _(str) but with format string checking
//
// Format strings in Taisei must either be a literal or F_(literal).
// When compiling with clang, this is enforced by -Werror=format-nonliteral.
// Unfortunately, GCC only catches cases without formatting arguments and canot be relied upon.
#define F_(str) i18n_translate_format(str)
