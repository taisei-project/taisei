/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <string.h>
#include <assert.h>

#include "config.h"
#include "global.h"
#include "paths/native.h"
#include "taisei_err.h"

static bool config_initialized = false;

CONFIGDEFS_EXPORT ConfigEntry configdefs[] = {
	#define CONFIGDEF(type,entryname,default,ufield) {type, entryname, {.ufield = default}, NULL},
	#define CONFIGDEF_KEYBINDING(id,entryname,default) CONFIGDEF(CONFIG_TYPE_KEYBINDING, entryname, default, i)
	#define CONFIGDEF_GPKEYBINDING(id,entryname,default) CONFIGDEF(CONFIG_TYPE_INT, entryname, default, i)
	#define CONFIGDEF_INT(id,entryname,default) CONFIGDEF(CONFIG_TYPE_INT, entryname, default, i)
	#define CONFIGDEF_FLOAT(id,entryname,default) CONFIGDEF(CONFIG_TYPE_FLOAT, entryname, default, f)
	#define CONFIGDEF_STRING(id,entryname,default) CONFIGDEF(CONFIG_TYPE_STRING, entryname, NULL, s)

	CONFIGDEFS
	{0}

	#undef CONFIGDEF
	#undef CONFIGDEF_KEYBINDING
	#undef CONFIGDEF_GPKEYBINDING
	#undef CONFIGDEF_INT
	#undef CONFIGDEF_FLOAT
	#undef CONFIGDEF_STRING
};

typedef struct ConfigEntryList {
	struct ConfigEntryList *next;
	struct ConfigEntryList *prev;
	ConfigEntry entry;
} ConfigEntryList;

static ConfigEntryList *unknowndefs = NULL;

void config_init(void) {
	if(config_initialized) {
		return;
	}

	#define CONFIGDEF_KEYBINDING(id,entryname,default)
	#define CONFIGDEF_GPKEYBINDING(id,entryname,default)
	#define CONFIGDEF_INT(id,entryname,default)
	#define CONFIGDEF_FLOAT(id,entryname,default)
	#define CONFIGDEF_STRING(id,entryname,default) stralloc(&configdefs[CONFIG_##id].val.s, default);

	CONFIGDEFS

	#undef CONFIGDEF_KEYBINDING
	#undef CONFIGDEF_GPKEYBINDING
	#undef CONFIGDEF_INT
	#undef CONFIGDEF_FLOAT
	#undef CONFIGDEF_STRING

	config_initialized = true;
}

static void config_delete_unknown_entries(void);

void config_uninit(void) {
	for(ConfigEntry *e = configdefs; e->name; ++e) {
		if(e->type == CONFIG_TYPE_STRING) {
			free(e->val.s);
			e->val.s = NULL;
		}
	}

	config_delete_unknown_entries();
}

void config_reset(void) {
	#define CONFIGDEF(id,default,ufield) configdefs[CONFIG_##id].val.ufield = default;
	#define CONFIGDEF_KEYBINDING(id,entryname,default) CONFIGDEF(id, default, i)
	#define CONFIGDEF_GPKEYBINDING(id,entryname,default) CONFIGDEF(id, default, i)
	#define CONFIGDEF_INT(id,entryname,default) CONFIGDEF(id, default, i)
	#define CONFIGDEF_FLOAT(id,entryname,default) CONFIGDEF(id, default, f)
	#define CONFIGDEF_STRING(id,entryname,default) stralloc(&configdefs[CONFIG_##id].val.s, default);

	CONFIGDEFS

	#undef CONFIGDEF
	#undef CONFIGDEF_KEYBINDING
	#undef CONFIGDEF_GPKEYBINDING
	#undef CONFIGDEF_INT
	#undef CONFIGDEF_FLOAT
	#undef CONFIGDEF_STRING
}

#ifndef CONFIG_RAWACCESS
ConfigEntry* config_get(ConfigIndex idx) {
	assert(idx >= 0 && idx < CONFIGIDX_NUM);
	return configdefs + idx;
}
#endif

static ConfigEntry* config_find_entry(const char *name) {
	ConfigEntry *e = configdefs;
	do if(!strcmp(e->name, name)) return e; while((++e)->name);
	return NULL;
}

KeyIndex config_key_from_scancode(int scan) {
	for(int i = CONFIG_KEY_FIRST; i <= CONFIG_KEY_LAST; ++i) {
		if(configdefs[i].val.i == scan) {
			return CFGIDX_TO_KEYIDX(i);
		}
	}

	return -1;
}

GamepadKeyIndex config_gamepad_key_from_gamepad_button(int btn) {
	for(int i = CONFIG_GAMEPAD_KEY_FIRST; i <= CONFIG_GAMEPAD_KEY_LAST; ++i) {
		if(configdefs[i].val.i == btn) {
			return CFGIDX_TO_GPKEYIDX(i);
		}
	}

	return -1;
}

KeyIndex config_key_from_gamepad_key(GamepadKeyIndex gpkey) {
	#define CONFIGDEF_GPKEYBINDING(id,entryname,default) case GAMEPAD_##id: return id;
	switch(gpkey) {
		GPKEYDEFS
		default: return -1;
	}
	#undef CONFIGDEF_GPKEYBINDING
}

GamepadKeyIndex config_gamepad_key_from_key(KeyIndex key) {
	#define CONFIGDEF_GPKEYBINDING(id,entryname,default) case id: return GAMEPAD_##id;
	switch(key) {
		GPKEYDEFS
		default: return -1;
	}
	#undef CONFIGDEF_GPKEYBINDING
}

KeyIndex config_key_from_gamepad_button(int btn) {
	return config_key_from_gamepad_key(config_gamepad_key_from_gamepad_button(btn));
}

static void config_set_val(ConfigIndex idx, ConfigValue v) {
	ConfigEntry *e = config_get(idx);
	ConfigCallback callback = e->callback;
	bool difference = true;

	switch(e->type) {
		case CONFIG_TYPE_INT:
		case CONFIG_TYPE_KEYBINDING:
			difference = e->val.i != v.i;
			break;

		case CONFIG_TYPE_FLOAT:
			difference = e->val.f != v.f;
			break;

		case CONFIG_TYPE_STRING:
			difference = strcmp(e->val.s, v.s);
			break;
	}

	if(!difference) {
		return;
	}

	if(callback) {
		e->callback = NULL;
		callback(idx, v);
		e->callback = callback;
		return;
	}

#ifdef DEBUG
	#define PRINTVAL(t) printf("config_set_val(): %s:" #t " = %" #t "\n", e->name, e->val.t);
#else
	#define PRINTVAL(t)
#endif

	switch(e->type) {
		case CONFIG_TYPE_INT:
		case CONFIG_TYPE_KEYBINDING:
			e->val.i = v.i;
			PRINTVAL(i)
			break;

		case CONFIG_TYPE_FLOAT:
			e->val.f = v.f;
			PRINTVAL(f)
			break;

		case CONFIG_TYPE_STRING:
			stralloc(&e->val.s, v.s);
			PRINTVAL(s)
			break;
	}

#undef PRINTVAL
}

int config_set_int(ConfigIndex idx, int val) {
	ConfigValue v = {.i = val};
	config_set_val(idx, v);
	return config_get_int(idx);
}

double config_set_float(ConfigIndex idx, double val) {
	ConfigValue v = {.f = val};
	config_set_val(idx, v);
	return config_get_float(idx);
}

char* config_set_str(ConfigIndex idx, const char *val) {
	ConfigValue v = {.s = (char*)val};
	config_set_val(idx, v);
	return config_get_str(idx);
}

void config_set_callback(ConfigIndex idx, ConfigCallback callback) {
	ConfigEntry *e = config_get(idx);
	assert(e->callback == NULL);
	e->callback = callback;
}

static ConfigEntry* config_get_unknown_entry(const char *name) {
	ConfigEntry *e;
	ConfigEntryList *l;

	for(l = unknowndefs; l; l = l->next) {
		if(!strcmp(name, l->entry.name)) {
			return &l->entry;
		}
	}

	l = create_element((void**)&unknowndefs, sizeof(ConfigEntryList));
	e = &l->entry;
	memset(e, 0, sizeof(ConfigEntry));
	stralloc(&e->name, name);
	e->type = CONFIG_TYPE_STRING;

	return e;
}

static void config_set_unknown(const char *name, const char *val) {
	stralloc(&config_get_unknown_entry(name)->val.s, val);
}

static void config_delete_unknown_entry(void **list, void *lentry) {
	ConfigEntry *e = &((ConfigEntryList*)lentry)->entry;

	free(e->name);
	free(e->val.s);

	delete_element(list, lentry);
}

static void config_delete_unknown_entries(void) {
	delete_all_elements((void**)&unknowndefs, config_delete_unknown_entry);
}

static char* config_path(const char *filename) {
	return strjoin(get_config_path(), "/", filename, NULL);
}

static SDL_RWops* config_open(const char *filename, const char *mode) {
	char *buf = config_path(filename);
	SDL_RWops *out = SDL_RWFromFile(buf, mode);

	free(buf);

	if(!out) {
		warnx("config_open(): SDL_RWFromFile() failed: %s", SDL_GetError());
	}

	return out;
}

void config_save(const char *filename) {
	SDL_RWops *out = config_open(filename, "w");
	ConfigEntry *e = configdefs;

	if(!out)
		return;

	SDL_RWprintf(out, "# Generated by taisei\n");

	do switch(e->type) {
		case CONFIG_TYPE_INT:
			SDL_RWprintf(out, "%s = %i\n", e->name, e->val.i);
			break;

		case CONFIG_TYPE_KEYBINDING:
			SDL_RWprintf(out, "%s = %s\n", e->name, SDL_GetScancodeName(e->val.i));
			break;

		case CONFIG_TYPE_STRING:
			SDL_RWprintf(out, "%s = %s\n", e->name, e->val.s);
			break;

		case CONFIG_TYPE_FLOAT:
			SDL_RWprintf(out, "%s = %f\n", e->name, e->val.f);
			break;
	} while((++e)->name);

	if(unknowndefs) {
		SDL_RWprintf(out, "# The following options were not recognized by taisei\n");

		for(ConfigEntryList *l = unknowndefs; l; l = l->next) {
			e = &l->entry;
			SDL_RWprintf(out, "%s = %s\n", e->name, e->val.s);
		}
	}

	SDL_RWclose(out);
	printf("Saved config '%s'\n", filename);
}

#define SYNTAXERROR { warnx("config_load(): syntax error on line %i, aborted! [%s:%i]\n", line, __FILE__, __LINE__); goto end; }
#define BUFFERERROR { warnx("config_load(): string exceed the limit of %i, aborted! [%s:%i]", CONFIG_LOAD_BUFSIZE, __FILE__, __LINE__); goto end; }
#define INTOF(s)   ((int)strtol(s, NULL, 10))
#define FLOATOF(s) ((float)strtod(s, NULL))

static void config_set(const char *key, const char *val, void *data) {
	ConfigEntry *e = config_find_entry(key);

	if(!e) {
		warnx("config_set(): unknown setting '%s'", key);
		config_set_unknown(key, val);
		return;
	}

	switch(e->type) {
		case CONFIG_TYPE_INT:
			e->val.i = INTOF(val);
			break;

		case CONFIG_TYPE_KEYBINDING: {
			SDL_Scancode scan = SDL_GetScancodeFromName(val);

			if(scan == SDL_SCANCODE_UNKNOWN) {
				warnx("config_set(): unknown key '%s'", val);
			} else {
				e->val.i = scan;
			}

			break;
		}

		case CONFIG_TYPE_STRING:
			stralloc(&e->val.s, val);
			break;

		case CONFIG_TYPE_FLOAT:
			e->val.f = FLOATOF(val);
			break;
	}
}

#undef INTOF
#undef FLOATOF

void config_load(const char *filename) {
	char *path = config_path(filename);
	config_init();
	parse_keyvalue_file_cb(path, config_set, NULL);
	free(path);
}

#undef SYNTAXERROR
#undef BUFFERERROR
