/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "config.h"
#include "global.h"
#include "version.h"
#include "util/strbuf.h"

static bool config_initialized = false;

CONFIGDEFS_EXPORT ConfigEntry configdefs[] = {
	#define CONFIGDEF(type,entryname,default,ufield) { type, entryname, { .ufield = default } },
	#define CONFIGDEF_KEYBINDING(id,entryname,default) CONFIGDEF(CONFIG_TYPE_KEYBINDING, entryname, default, i)
	#define CONFIGDEF_GPKEYBINDING(id,entryname,default) CONFIGDEF(CONFIG_TYPE_GPKEYBINDING, entryname, default, i)
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
	LIST_INTERFACE(struct ConfigEntryList);
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

static void config_copy_value(ConfigEntryType type, ConfigValue *dst, ConfigValue src) {
	switch(type) {
		case CONFIG_TYPE_INT:
		case CONFIG_TYPE_KEYBINDING:
		case CONFIG_TYPE_GPKEYBINDING:
			dst->i = src.i;
			break;

		case CONFIG_TYPE_FLOAT:
			dst->f = src.f;
			break;

		case CONFIG_TYPE_STRING:
			stralloc(&dst->s, src.s);
			break;

		default:
			UNREACHABLE;
	}
}

static int config_compare_values(ConfigEntryType type, ConfigValue val0, ConfigValue val1) {
	switch(type) {
		case CONFIG_TYPE_INT:
		case CONFIG_TYPE_KEYBINDING:
		case CONFIG_TYPE_GPKEYBINDING:
			return (val0.i > val1.i) - (val0.i < val1.i);

		case CONFIG_TYPE_FLOAT:
			return (val0.f > val1.f) - (val0.f < val1.f);

		case CONFIG_TYPE_STRING:
			return strcmp(val0.s, val1.s);

		default:
			UNREACHABLE;
	}
}

static void config_free_value(ConfigEntryType type, ConfigValue *val) {
	switch(type) {
		case CONFIG_TYPE_STRING:
			mem_free(val->s);
			val->s = NULL;
			break;

		default:
			break;
	}
}

void config_shutdown(void) {
	for(ConfigEntry *e = configdefs; e->name; ++e) {
		config_free_value(e->type, &e->val);
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

static void config_dump_stringval(StringBuffer *buf, ConfigEntryType etype, ConfigValue *val) {
	switch(etype) {
		case CONFIG_TYPE_INT:
			strbuf_printf(buf, "%i", val->i);
			break;

		case CONFIG_TYPE_KEYBINDING:
			strbuf_cat(buf, SDL_GetScancodeName(val->i));
			break;

		case CONFIG_TYPE_GPKEYBINDING:
			strbuf_cat(buf, gamepad_button_name(val->i));
			break;

		case CONFIG_TYPE_STRING:
			strbuf_cat(buf, val->s);
			break;

		case CONFIG_TYPE_FLOAT:
			strbuf_printf(buf, "%f", val->f);
			break;

		default: UNREACHABLE;
	}
}

static void config_set_val(ConfigIndex idx, ConfigValue v) {
	ConfigEntry *e = config_get(idx);

	if(!config_compare_values(e->type, e->val, v)) {
		return;
	}

	ConfigValue oldv = { 0 };
	ConfigEntryType ctype = e->type;
	config_copy_value(ctype, &oldv, e->val);
	config_copy_value(ctype, &e->val, v);

#ifdef DEBUG
	StringBuffer sb = {};
	strbuf_printf(&sb, "%s: [", e->name);
	config_dump_stringval(&sb, ctype, &oldv);
	strbuf_printf(&sb, "] -> [");
	config_dump_stringval(&sb, ctype, &e->val);
	strbuf_printf(&sb, "]");
	log_debug("%s", sb.start);
	strbuf_free(&sb);
#endif

	events_emit(TE_CONFIG_UPDATED, idx, &e->val, &oldv);
	config_free_value(ctype, &oldv);
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

static ConfigEntry* config_get_unknown_entry(const char *name) {
	ConfigEntry *e;
	ConfigEntryList *l;

	for(l = unknowndefs; l; l = l->next) {
		if(!strcmp(name, l->entry.name)) {
			return &l->entry;
		}
	}

	l = ALLOC(typeof(*l));
	e = &l->entry;
	stralloc(&e->name, name);
	e->type = CONFIG_TYPE_STRING;
	list_push(&unknowndefs, l);

	return e;
}

static void config_set_unknown(const char *name, const char *val) {
	stralloc(&config_get_unknown_entry(name)->val.s, val);
}

static void* config_delete_unknown_entry(List **list, List *lentry, void *arg) {
	ConfigEntry *e = &((ConfigEntryList*)lentry)->entry;

	mem_free(e->name);
	mem_free(e->val.s);
	mem_free(list_unlink(list, lentry));

	return NULL;
}

static void config_delete_unknown_entries(void) {
	list_foreach(&unknowndefs, config_delete_unknown_entry, NULL);
}

void config_save(void) {
	SDL_RWops *out = vfs_open(CONFIG_FILE, VFS_MODE_WRITE);
	ConfigEntry *e = configdefs;

	if(!out) {
		log_error("VFS error: %s", vfs_get_error());
		return;
	}

	StringBuffer sbuf = {};

	strbuf_printf(&sbuf, "# Generated by %s %s", TAISEI_VERSION_FULL, TAISEI_VERSION_BUILD_TYPE);

	do {
		strbuf_printf(&sbuf, "\n%s = ", e->name);
		config_dump_stringval(&sbuf, e->type, &e->val);
	} while((++e)->name);

	if(unknowndefs) {
		strbuf_cat(&sbuf, "\n# The following options were not recognized by taisei");

		for(ConfigEntryList *l = unknowndefs; l; l = l->next) {
			e = &l->entry;
			strbuf_printf(&sbuf, "\n%s = %s", e->name, e->val.s);
		}
	}

	SDL_RWwrite(out, sbuf.start, sbuf.pos - sbuf.start, 1);
	strbuf_free(&sbuf);
	SDL_RWclose(out);

	char *sp = vfs_repr(CONFIG_FILE, true);
	log_info("Saved config '%s'", sp);
	mem_free(sp);
}

#define INTOF(s)   ((int)strtol(s, NULL, 10))
#define FLOATOF(s) ((float)strtod(s, NULL))

typedef struct ConfigParseState {
	int first_entry;
} ConfigParseState;

static bool config_set(const char *key, const char *val, void *data) {
	ConfigEntry *e = config_find_entry(key);
	ConfigParseState *state = data;

	if(!e) {
		log_warn("Unknown setting '%s'", key);
		config_set_unknown(key, val);

		if(state->first_entry < 0) {
			state->first_entry = CONFIGIDX_NUM;
		}

		return true;
	}

	if(state->first_entry < 0) {
		state->first_entry = (intptr_t)(e - configdefs);
	}

	switch(e->type) {
		case CONFIG_TYPE_INT:
			e->val.i = INTOF(val);
			break;

		case CONFIG_TYPE_KEYBINDING: {
			SDL_Scancode scan = SDL_GetScancodeFromName(val);

			if(scan == SDL_SCANCODE_UNKNOWN) {
				log_warn("Unknown key '%s'", val);
			} else {
				e->val.i = scan;
			}

			break;
		}

		case CONFIG_TYPE_GPKEYBINDING: {
			GamepadButton btn = gamepad_button_from_name(val);

			if(btn == GAMEPAD_BUTTON_INVALID) {
				log_warn("Unknown gamepad button '%s'", val);
			} else {
				e->val.i = btn;
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

	return true;
}

#undef INTOF
#undef FLOATOF

typedef void (*ConfigUpgradeFunc)(void);

static void config_upgrade_1(void) {
	// reset vsync to the default value
	config_set_int(CONFIG_VSYNC, CONFIG_VSYNC_DEFAULT);

	// this version also changes meaning of the vsync value
	// previously it was: 0 = on,  1 = off, 2 = adaptive, because mia doesn't know how my absolutely genius options menu works.
	//         now it is: 0 = off, 1 = on,  2 = adaptive, as it should be.
}

#ifdef __EMSCRIPTEN__
static void config_upgrade_2(void) {
	// emscripten defaults for these have been changed
	config_set_int(CONFIG_VSYNC, CONFIG_VSYNC_DEFAULT);
	config_set_int(CONFIG_FXAA, CONFIG_FXAA_DEFAULT);
}
#else
#define config_upgrade_2 NULL
#endif

static ConfigUpgradeFunc config_upgrades[] = {
	/*
		To bump the config version and add an upgrade state, simply append an upgrade function to this array.
		NEVER reorder these entries, remove them, etc, ONLY append.

		If a stage is no longer needed (nothing useful needs to be done, e.g. the later stages would override it),
		then replace the function pointer with NULL.
	*/

	config_upgrade_1,
	config_upgrade_2,
};

static void config_apply_upgrades(int start) {
	int num_upgrades = sizeof(config_upgrades) / sizeof(ConfigUpgradeFunc);

	if(start > num_upgrades) {
		log_warn("Config seems to be from a newer version of Taisei (config version %i, ours is %i)", start, num_upgrades);
		return;
	}

	for(int upgrade = start; upgrade < num_upgrades; ++upgrade) {
		if(config_upgrades[upgrade]) {
			log_info("Upgrading config to version %i", upgrade + 1);
			config_upgrades[upgrade]();
		} else {
			log_debug("Upgrade to version %i is a no-op", upgrade + 1);
		}
	}
}

void config_load(void) {
	config_init();

	char *config_path = vfs_repr(CONFIG_FILE, true);
	SDL_RWops *config_file = vfs_open(CONFIG_FILE, VFS_MODE_READ);

	if(config_file) {
		log_info("Loading configuration from %s", config_path);

		auto state = ALLOC(ConfigParseState);
		state->first_entry = -1;

		if(!parse_keyvalue_stream_cb(config_file, config_set, state)) {
			log_warn("Errors occured while parsing the configuration file");
		}

		if(state->first_entry != CONFIG_VERSION) {
			// config file was likely written by an old version of taisei that is unaware of the upgrade mechanism.
			config_set_int(CONFIG_VERSION, 0);
		}

		mem_free(state);
		SDL_RWclose(config_file);

		config_apply_upgrades(config_get_int(CONFIG_VERSION));
	} else {
		VFSInfo i = vfs_query(CONFIG_FILE);

		if(i.error) {
			log_error("VFS error: %s", vfs_get_error());
		} else if(i.exists) {
			log_error("Config file %s is not readable", config_path);
		}
	}

	mem_free(config_path);

	// set config version to the latest
	config_set_int(CONFIG_VERSION, sizeof(config_upgrades) / sizeof(ConfigUpgradeFunc));

#ifdef __SWITCH__
	config_set_int(CONFIG_GAMEPAD_ENABLED, true);
	config_set_str(CONFIG_GAMEPAD_DEVICE, "any");
#endif
}
