/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once

typedef struct ht_str2trans_t ht_str2trans_t;

typedef struct IntlTextDomain {
	char *strings;
	ht_str2trans_t *table;
} IntlTextDomain;

IntlTextDomain *intl_read_mo(const char* path);
void intl_textdomain_destroy(IntlTextDomain *textdomain);

void intl_set_textdomain(IntlTextDomain *textdomain);

const char *intl_gettext(IntlTextDomain *domain, const char *msgid);
const char *intl_ngettext(IntlTextDomain *domain, const char *msgid1, const char *msgid2, unsigned long int n);

const char *gettext(const char *msgid);
const char *ngettext(const char *msgid1, const char *msgid2, unsigned long int n);
#define pgettext(msgctxt, msgid) gettext(msgctxt "\004" msgid)

#define _(str) gettext(str)
// N_(str) marks strings for xgettext without translating them
#define N_(str) str

