/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "config.h"

#include "bitarray.h"
#include "gamepad.h"
#include "global.h"   // IWYU pragma: keep
#include "util/kvparser.h"
#include "util/strbuf.h"
#include "version.h"
#include "vfs/public.h"

#define CONFIG_FILE "storage/config"

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
			return val0.s
					? (val1.s ? strcmp(val0.s, val1.s) : -1)
					: (val1.s ? 1 : 0);

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
			strbuf_cat(buf, val->s ?: "(null)");
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

	ConfigValue oldv = {};
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

	if(SDL_WasInit(SDL_INIT_EVENTS)) {
		events_emit(TE_CONFIG_UPDATED, idx, &e->val, &oldv);
	}

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
	SDL_IOStream *out = vfs_open(CONFIG_FILE, VFS_MODE_WRITE);
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

	SDL_WriteIO(out, sbuf.start, (sbuf.pos - sbuf.start));
	strbuf_free(&sbuf);
	SDL_CloseIO(out);

	char *sp = vfs_repr(CONFIG_FILE, true);
	log_info("Saved config '%s'", sp);
	mem_free(sp);
}

#define INTOF(s)   ((int)strtol(s, NULL, 10))
#define FLOATOF(s) ((float)strtod(s, NULL))

typedef struct ConfigParseState {
	BIT_ARRAY(CONFIGIDX_NUM) touched_settings;
} ConfigParseState;

static bool config_set(const char *key, const char *val, void *data) {
	ConfigEntry *e = config_find_entry(key);
	ConfigParseState *state = data;

	if(!e) {
		log_warn("Unknown setting '%s'", key);
		config_set_unknown(key, val);

		return true;
	}

	ConfigIndex idx = e - configdefs;
	assert((uintptr_t)idx < CONFIGIDX_NUM);
	bitarray_set(&state->touched_settings, idx, true);

	bool error = false;
	ConfigValue cval = { };

	switch(e->type) {
		case CONFIG_TYPE_INT:
			cval.i = INTOF(val);
			break;

		case CONFIG_TYPE_FLOAT:
			cval.f = FLOATOF(val);
			break;

		case CONFIG_TYPE_STRING:
			cval.s = (char*)val;
			break;

		case CONFIG_TYPE_KEYBINDING: {
			SDL_Scancode scan = SDL_GetScancodeFromName(val);

			if(scan == SDL_SCANCODE_UNKNOWN) {
				log_warn("Unknown key '%s'", val);
				error = true;
			} else {
				cval.i = scan;
			}

			break;
		}

		case CONFIG_TYPE_GPKEYBINDING: {
			GamepadButton btn = gamepad_button_from_name(val);

			if(btn == GAMEPAD_BUTTON_INVALID) {
				log_warn("Unknown gamepad button '%s'", val);
				error = true;
			} else {
				cval.i = btn;
			}

			break;
		}

		default: UNREACHABLE;
	}

	if(!error) {
		config_set_val(idx, cval);
	}

	return true;
}

#undef INTOF
#undef FLOATOF

typedef void (*ConfigUpgradeFunc)(ConfigParseState *state);

static void config_upgrade_1(ConfigParseState *state) {
	// reset vsync to the default value
	if(bitarray_get(&state->touched_settings, CONFIG_VSYNC)) {
		config_set_int(CONFIG_VSYNC, CONFIG_VSYNC_DEFAULT);
	}

	// this version also changes meaning of the vsync value
	// previously it was: 0 = on,  1 = off, 2 = adaptive, because mia doesn't know how my absolutely genius options menu works.
	//         now it is: 0 = off, 1 = on,  2 = adaptive, as it should be.
}

#ifdef __EMSCRIPTEN__
static void config_upgrade_2(ConfigParseState *state) {
	// emscripten defaults for these have been changed
	if(bitarray_get(&state->touched_settings, CONFIG_VSYNC)) {
		config_set_int(CONFIG_VSYNC, CONFIG_VSYNC_DEFAULT);
	}

	if(bitarray_get(&state->touched_settings, CONFIG_FXAA)) {
		config_set_int(CONFIG_FXAA, CONFIG_FXAA_DEFAULT);
	}
}
#else
#define config_upgrade_2 NULL
#endif

static void config_upgrade_3(ConfigParseState *state) {
	if(bitarray_get(&state->touched_settings, CONFIG_MIXER_CHUNKSIZE)) {
		config_set_int(CONFIG_MIXER_CHUNKSIZE, CONFIG_CHUNKSIZE_DEFAULT);
	}
}

static void config_upgrade_4(ConfigParseState *state) {
	if(bitarray_get(&state->touched_settings, CONFIG_GAMEPAD_BTNREPEAT_DELAY)) {
		config_set_float(CONFIG_GAMEPAD_BTNREPEAT_DELAY, CONFIG_GAMEPAD_BTNREPEAT_DELAY_DEFAULT);
	}

	if(bitarray_get(&state->touched_settings, CONFIG_GAMEPAD_BTNREPEAT_INTERVAL)) {
		config_set_float(CONFIG_GAMEPAD_BTNREPEAT_INTERVAL, CONFIG_GAMEPAD_BTNREPEAT_INTERVAL_DEFAULT);
	}
}

static ConfigUpgradeFunc config_upgrades[] = {
	/*
		To bump the config version and add an upgrade state, simply append an upgrade function to this array.
		NEVER reorder these entries, remove them, etc, ONLY append.

		If a stage is no longer needed (nothing useful needs to be done, e.g. the later stages would override it),
		then replace the function pointer with NULL.
	*/

	config_upgrade_1,
	config_upgrade_2,
	config_upgrade_3,
	config_upgrade_4,
};

static void config_set_version_latest(void) {
	config_set_int(CONFIG_VERSION, sizeof(config_upgrades) / sizeof(ConfigUpgradeFunc));
}

static void config_apply_upgrades(int start, ConfigParseState *state) {
	int num_upgrades = sizeof(config_upgrades) / sizeof(ConfigUpgradeFunc);

	if(start > num_upgrades) {
		log_warn("Config seems to be from a newer version of Taisei (config version %i, ours is %i)", start, num_upgrades);
		return;
	}

	for(int upgrade = start; upgrade < num_upgrades; ++upgrade) {
		if(config_upgrades[upgrade]) {
			log_info("Upgrading config to version %i", upgrade + 1);
			config_upgrades[upgrade](state);
		} else {
			log_debug("Upgrade to version %i is a no-op", upgrade + 1);
		}
	}

	config_set_version_latest();
}

static bool config_load_stream(SDL_IOStream *stream) {
	config_set_int(CONFIG_VERSION, 0);
	ConfigParseState state = {};

	if(!parse_keyvalue_stream_cb(stream, config_set, &state)) {
		log_warn("Errors occurred while parsing the configuration file");
	}

	config_apply_upgrades(config_get_int(CONFIG_VERSION), &state);
	return true;
}

static bool config_load_file(const char *vfspath, LogLevel fail_loglevel) {
	SDL_IOStream *stream = vfs_open(vfspath, VFS_MODE_READ);
	char *syspath_alloc = vfs_repr(vfspath, true);
	const char *syspath = syspath_alloc ?: vfspath;
	log_info("Loading configuration from %s", syspath);

	if(UNLIKELY(!stream)) {
		log_custom(fail_loglevel, "VFS error: %s", vfs_get_error());

		if(vfs_query(vfspath).exists) {
			log_error("Config file %s exists but is not readable", syspath);
		}

		mem_free(syspath_alloc);
		return false;
	}

	mem_free(syspath_alloc);
	bool ok = config_load_stream(stream);
	SDL_CloseIO(stream);
	return ok;
}

void config_load(void) {
	config_reset();
	config_load_file(CONFIG_FILE, LOG_INFO);

#ifdef __SWITCH__
	config_set_int(CONFIG_GAMEPAD_ENABLED, true);
	config_set_str(CONFIG_GAMEPAD_DEVICE, "any");
	config_set_int(CONFIG_VID_WIDTH, nxGetInitialScreenWidth());
	config_set_int(CONFIG_VID_HEIGHT, nxGetInitialScreenHeight());
#endif
}

void config_reset(void) {
	#define CONFIGDEF(id, default, type) config_set_##type(CONFIG_##id, default);
	#define CONFIGDEF_KEYBINDING(id, entryname, default) CONFIGDEF(id, default, int)
	#define CONFIGDEF_GPKEYBINDING(id, entryname, default) CONFIGDEF(GAMEPAD_##id, default, int)
	#define CONFIGDEF_INT(id, entryname, default) CONFIGDEF(id, default, int)
	#define CONFIGDEF_FLOAT(id, entryname, default) CONFIGDEF(id, default, float)
	#define CONFIGDEF_STRING(id, entryname, default) CONFIGDEF(id, default, str)

	CONFIGDEFS

	#undef CONFIGDEF
	#undef CONFIGDEF_KEYBINDING
	#undef CONFIGDEF_GPKEYBINDING
	#undef CONFIGDEF_INT
	#undef CONFIGDEF_FLOAT
	#undef CONFIGDEF_STRING

	config_set_version_latest();
	config_load_file("res/config.default", LOG_INFO);
}
