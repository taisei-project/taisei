/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "hashtable.h"

typedef struct IntlTranslation {
	char *text;
	char *plural;
} IntlTranslation;

// text can be either "text" or "plural\0singular"
void intl_translation_create(IntlTranslation *translation, const char *text, int length);
void intl_translation_destroy(IntlTranslation *translation);

typedef struct IntlTextDomain {
	int number_of_strings;
	IntlTranslation *translated_strings;

	ht_str2ptr_ts_t table;
} IntlTextDomain;

IntlTextDomain *intl_textdomain_new(int number_of_strings, char **original_strings, IntlTranslation* translated_strings);
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

