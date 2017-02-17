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

void config_uninit(void) {
	for(ConfigEntry *e = configdefs; e->name; ++e) {
		if(e->type == CONFIG_TYPE_STRING && e->val.s) {
			free(e->val.s);
			e->val.s = NULL;
		}
	}
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

static ConfigEntry* config_find_entry(char *name) {
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

static FILE* config_open(char *filename, char *mode) {
	char *buf;
	FILE *out;

	buf = malloc(strlen(filename) + strlen(get_config_path()) + 2);
	strcpy(buf, get_config_path());
	strcat(buf, "/");
	strcat(buf, filename);

	out = fopen(buf, mode);
	free(buf);

	if(!out) {
		warnx("config_open(): couldn't open '%s'", filename);
		return NULL;
	}

	return out;
}

void config_save(char *filename) {
	FILE *out = config_open(filename, "w");
	ConfigEntry *e = configdefs;

	if(!out)
		return;

	fputs("# Generated by taisei\n", out);

	do switch(e->type) {
		case CONFIG_TYPE_INT:
			fprintf(out, "%s = %i\n", e->name, e->val.i);
			break;

		case CONFIG_TYPE_KEYBINDING:
			fprintf(out, "%s = %s\n", e->name, SDL_GetScancodeName(e->val.i));
			break;

		case CONFIG_TYPE_STRING:
			fprintf(out, "%s = %s\n", e->name, e->val.s);
			break;

		case CONFIG_TYPE_FLOAT:
			fprintf(out, "%s = %f\n", e->name, e->val.f);
			break;
	} while((++e)->name);

	fclose(out);
	printf("Saved config '%s'\n", filename);
}

#define SYNTAXERROR { warnx("config_load(): syntax error on line %i, aborted! [%s:%i]\n", line, __FILE__, __LINE__); goto end; }
#define BUFFERERROR { warnx("config_load(): string exceed the limit of %i, aborted! [%s:%i]", CONFIG_LOAD_BUFSIZE, __FILE__, __LINE__); goto end; }
#define INTOF(s)   ((int)strtol(s, NULL, 10))
#define FLOATOF(s) ((float)strtod(s, NULL))

static void config_set(char *key, char *val) {
	ConfigEntry *e = config_find_entry(key);

	if(!e) {
		warnx("config_set(): unknown setting '%s'", key);
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

void config_load(char *filename) {
	FILE *in = config_open(filename, "r");
	int c, i = 0, found, line = 0;
	char buf[CONFIG_LOAD_BUFSIZE];
	char key[CONFIG_LOAD_BUFSIZE];
	char val[CONFIG_LOAD_BUFSIZE];

	config_init();

	if(!in) {
		return;
	}

	while((c = fgetc(in)) != EOF) {
		if(c == '#' && !i) {
			i = 0;
			while(fgetc(in) != '\n');
		} else if(c == ' ') {
			if(!i) SYNTAXERROR

			buf[i] = 0;
			i = 0;
			strcpy(key, buf);

			found = 0;
			while((c = fgetc(in)) != EOF) {
				if(c == '=') {
					if(++found > 1) SYNTAXERROR
				} else if(c != ' ') {
					if(!found || c == '\n') SYNTAXERROR

					do {
						if(c == '\n') {
							if(!i) SYNTAXERROR

							buf[i] = 0;
							i = 0;
							strcpy(val, buf);
							found = 0;
							++line;

							config_set(key, val);
							break;
						} else {
							buf[i++] = c;
							if(i == CONFIG_LOAD_BUFSIZE)
								BUFFERERROR
						}
					} while((c = fgetc(in)) != EOF);

					break;
				}
			} if(found) SYNTAXERROR
		} else {
			buf[i++] = c;
			if(i == CONFIG_LOAD_BUFSIZE)
				BUFFERERROR
		}
	}

end:
	fclose(in);
}

#undef SYNTAXERROR
#undef BUFFERERROR
