/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "util/compat.h"
typedef struct ht_str2trans_t ht_str2trans_t;

typedef struct IntlTextDomain {
	char *strings;
	ht_str2trans_t *table;
} IntlTextDomain;

extern IntlTextDomain *_intl_current_textdomain;

IntlTextDomain *intl_read_mo(const char* path);
void intl_textdomain_destroy(IntlTextDomain *textdomain);

void intl_set_textdomain(IntlTextDomain *textdomain);

const char *_intl_gettext_prehashed(IntlTextDomain *domain, const char *msgid, hash_t hash);
const char *_intl_ngettext_prehashed(IntlTextDomain *domain, const char *msgid1, hash_t hash1, const char *msgid2, unsigned long int n);

attr_format_arg(2) INLINE  const char *intl_gettext(IntlTextDomain *domain, const char *msgid) {
	return _intl_gettext_prehashed(domain, msgid, htutil_hashfunc_string(msgid));
}
attr_format_arg(2) INLINE const char *intl_ngettext(IntlTextDomain *domain, const char *msgid1, const char *msgid2, unsigned long int n) {
	return _intl_ngettext_prehashed(domain, msgid1, htutil_hashfunc_string(msgid1), msgid2, n);
}

attr_format_arg(1) INLINE const char *gettext(const char *msgid) {
	return intl_gettext(_intl_current_textdomain, msgid);
}

attr_format_arg(1) INLINE const char *ngettext(const char *msgid1, const char *msgid2, unsigned long int n) {
	return intl_ngettext(_intl_current_textdomain, msgid1, msgid2, n);
}

#define pgettext(msgctxt, msgid) gettext(msgctxt "\004" msgid)

#define _(str) gettext(str)
// N_(str) marks strings for xgettext without translating them
#define N_(str) str

