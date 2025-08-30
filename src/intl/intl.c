/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "hashtable.h"
#include "taisei.h"

#include "intl.h"

static IntlTextDomain *_current_textdomain = NULL;

void intl_translation_create(IntlTranslation *translation, const char *text, int length) {
	int textlength = strlen(text);
	char *buf = memdup(text, length+1);
		assert(textlength == length);

	if(textlength != length) {
		assert(textlength < length);
		translation->plural = buf;
		translation->text = buf + textlength + 1;
	} else {
		translation->text = buf;
	}
}

void intl_translation_destroy(IntlTranslation *translation) {
	if(!translation->plural) {
		mem_free(translation->text);
	} else {
		mem_free(translation->plural);
	}
}

static void translation_copy(IntlTranslation *dest, IntlTranslation *src) {
	if(!src->plural) {
		dest->text = mem_strdup(src->text);
		dest->plural = NULL;
	} else {
		int plural_length = src->text - src->plural;
		int length = strlen(src->text) + 1 + plural_length;
		dest->plural = memdup(src->plural, length);
		dest->text = dest->plural + plural_length;
	}
}

IntlTextDomain *intl_textdomain_new(int number_of_strings, char **original_strings, IntlTranslation* translated_strings) {
	IntlTextDomain *domain = mem_alloc(sizeof(IntlTextDomain));
	domain->number_of_strings = number_of_strings;
	domain->translated_strings = mem_alloc_array(number_of_strings, sizeof(IntlTranslation));

	ht_str2ptr_ts_create(&domain->table);
	for(int i = 0; i < number_of_strings; i++) {
		translation_copy(&domain->translated_strings[i], &translated_strings[i]);

		ht_str2ptr_ts_set(&domain->table, original_strings[i], &domain->translated_strings[i]);
	}
	return domain;
}

void intl_textdomain_destroy(IntlTextDomain *domain) {
	for(int i = 0; i < domain->number_of_strings; i++) {
		intl_translation_destroy(&domain->translated_strings[i]);
	}
	mem_free(domain->translated_strings);
	ht_str2ptr_ts_destroy(&domain->table);
}

void intl_set_textdomain(IntlTextDomain *domain) {
	_current_textdomain = domain;
}

const char *intl_gettext(IntlTextDomain *domain, const char *msgid) {
	if(!domain) {
		return msgid;
	}

	IntlTranslation *result = ht_str2ptr_ts_get(&domain->table, msgid, NULL);
	if(!result) {
		return msgid;
	}
	return result->text;
}

const char *intl_ngettext(IntlTextDomain *domain, const char *msgid1, const char *msgid2, unsigned long int n) {
	if(!domain) {
		return n == 1 ? msgid1 : msgid2;
	}

	IntlTranslation *result = ht_str2ptr_ts_get(&domain->table, msgid1, NULL);
	if(!result) {
		return n == 1 ? msgid1 : msgid2;
	}

	return n == 1 ? result->text : result->plural;
}

const char *gettext(const char *msgid) {
	return intl_gettext(_current_textdomain, msgid);
}

const char *ngettext(const char *msgid1, const char *msgid2, unsigned long int n) {
	return intl_ngettext(_current_textdomain, msgid1, msgid2, n);
}
