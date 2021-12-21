/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "events.h"
#include "resource/resource.h"
#include "taisei.h"

#include "charprofile.h"
#include "mainmenu.h"
#include "portrait.h"
#include "common.h"
#include "progress.h"
#include "options.h"
#include "global.h"
#include "portrait.h"
#include "util/glm.h"
#include "video.h"

#define DESCRIPTION_WIDTH (SCREEN_W / 3 + 80)
#define FACES(...) { __VA_ARGS__, NULL }

typedef enum {
	PROFILE_REIMU,
	PROFILE_MARISA,
	PROFILE_YOUMU,
	PROFILE_CIRNO,
	PROFILE_HINA,
	PROFILE_SCUTTLE,
	PROFILE_WRIGGLE,
	PROFILE_KURUMI,
	PROFILE_IKU,
	PROFILE_ELLY,
	PROFILE_LOCKED,
	NUM_PROFILES
} CharProfiles;

typedef struct CharacterProfile {
	const char *name;
	const char *fullname;
	const char *title;
	const char *description;
	const char *background;
	const char *faces[12];
	ProgressBGMID unlock;
} CharacterProfile;

typedef struct CharProfileContext {
	int8_t char_draw_order[NUM_PROFILES];
	int8_t prev_selected_char;
	int8_t face;
} CharProfileContext;

static const CharacterProfile profiles[NUM_PROFILES] = {
	[PROFILE_REIMU] = {
		.name = "reimu",
		.fullname = "Hakurei Reimu",
		.title = "Shrine Maiden of Fantasy",
		.description = "Species: Human\nArea of Study: Literature (Fiction)\n\nThe incredibly particular shrine maiden.\n\nShe’s taking a break from her busy novel-reading schedule to take care of an incident.\n\nMostly, she has a vague feeling of sentimentality for a bygone era.\n\nRegardless, when the residents of Yōkai Mountain show up sobbing at her door, she has no choice but to put her book down and spring into action.",
		.background = "reimubg",
		.faces = FACES("normal", "surprised", "unamused", "happy", "sigh", "smug", "puzzled", "assertive", "irritated", "outraged"),
	},
	[PROFILE_MARISA] = {
		.name = "marisa",
		.fullname = "Kirisame Marisa",
		.title = "Unbelievably Ordinary Magician",
		.description = "Species: Human\nArea of Study: Eclecticism\n\nThe confident, hyperactive witch.\n\nCurious as ever about the limits of magic and the nature of reality, and when she hears about “eldritch lunacy,” her curiosity gets the better of her.\n\nBut maybe this “grimoire” is best left unread?\n\nNot that a warning like that would stop her anyways. So, off she goes.",
		.background = "marisa_bombbg",
		.faces = FACES("normal", "surprised", "sweat_smile", "happy", "smug", "puzzled", "unamused", "inquisitive"),
	},
	[PROFILE_YOUMU] = {
		.name = "youmu",
		.fullname = "Konpaku Yōmu",
		.title = "Swordswoman Between Worlds",
		.description = "Species: Half-Human, Half-Phantom\nArea of Study: Fencing\n\nThe swordswoman of the somewhat-dead.\n\n“While you were playing games, I was studying the blade” … is probably what she’d say.\n\nEntirely too serious about everything, and utterly terrified of (half of) her own existence.\n\nStill, she’s been given an important mission by Lady Yuyuko. Hopefully she doesn’t let it get to her head.",
		.background = "youmu_bombbg1",
		.faces = FACES("normal", "eeeeh", "embarrassed", "eyes_closed", "chuuni", "happy", "relaxed", "sigh", "smug", "surprised", "unamused"),
	},
	[PROFILE_CIRNO] = {
		.name = "cirno",
		.fullname = "Cirno",
		.title = "Thermodynamic Ice Fairy",
		.description = "Species: Fairy\nArea of Study: Thermodynamics\n\nThe lovably-foolish ice fairy.\n\nPerhaps she’s a bit dissatisfied after her recent duels with secret gods and hellish fairies?\n\nConsider going easy on her.",
		.background = "stage1/cirnobg",
		.unlock = PBGM_STAGE1_BOSS,
		.faces = FACES("normal", "angry", "defeated"),
	},
	[PROFILE_HINA] = {
		.name = "hina",
		.fullname = "Kagiyama Hina",
		.title = "Gyroscopic Pestilence God",
		.description = "Species: Pestilence God\nArea of Study: Gyroscopic Stabilization\n\nGuardian of Yōkai Mountain. Her angular momentum is out of this world!\n\nShe seems awfully concerned with your health and safety, to the point of being quite overbearing.\n\nYou’re old enough to decide your own path in life, though.",
		.unlock = PBGM_STAGE2_BOSS,
		.background = "stage2/spellbg2",
		.faces = FACES("normal", "concerned", "serious", "defeated"),
	},
	[PROFILE_SCUTTLE] = {
		.name = "scuttle",
		.fullname = "Scuttle",
		.title = "Agile Insect Engineer",
		.description = "Species: Bombardier Beetle\nArea of Study: Full-Stack 'Web' Development\n\nNot all things are computable, and this bug seems to sneak in at a decisive fork in your commute to the true culprit.\n\nThe majority of insects do not scheme grand revolutions, yet their presence alone is enough to upset the course of events.",
		.unlock = PBGM_STAGE3_BOSS,
		.background = "stage3/spellbg1",
		.faces = FACES("normal"),
	},
	[PROFILE_WRIGGLE] = {
		.name = "wriggle",
		.fullname = "Wriggle Nightbug",
		.title = "Insect Rights Activist",
		.description = "Species: Insect\nArea of Study: Evolutionary Socio-Entomology\n\nA bright bug - or was it insect? - far from her usual stomping grounds.\nShe feels that Insects have lost their “rightful place” in history.\nIf she thinks she has a raw evolutionary deal, someone ought to tell her about the trilobites…",
		.unlock = PBGM_STAGE3_BOSS,
		.background = "stage3/spellbg2",
		.faces = FACES("normal", "calm", "outraged", "proud", "defeated"),
	},
	[PROFILE_KURUMI] = {
		.name = "kurumi",
		.fullname = "Kurumi",
		.title = "High-Society Phlebotomist",
		.description = "Species: Vampire\nArea of Study: Hematology\n\nVampiric blast from the past. Doesn’t she know Gensōkyō already has new bloodsuckers in town?\n\nSucking blood is what she’s good at, but her current job is a gatekeeper, and her true passion is high fashion.\n\nEveryone’s got multiple side-hustles these days…",
		.unlock = PBGM_STAGE4_BOSS,
		.background = "stage4/kurumibg1",
		.faces = FACES("normal", "dissatisfied", "puzzled", "tsun", "tsun_blush", "defeated"),
	},
	[PROFILE_IKU] = {
		.name = "iku",
		.fullname = "Nagae Iku",
		.title = "Fulminologist of the Heavens",
		.description = "Species: Oarfish\nArea of Study: Meteorology (Fulminology)\n\nMessenger from the clouds, and an amateur meteorologist.\n\nSeems to have been reading a few too many outdated psychiatry textbooks in recent times.\n\nDespite being so formal and stuffy, she seems to be the only one who knows what’s going on.",
		.background = "stage5/spell_lightning",
		.unlock = PBGM_STAGE5_BOSS,
		.faces = FACES("normal", "smile", "serious", "eyes_closed", "defeated"),
	},
	[PROFILE_ELLY] = {
		.name = "elly",
		.fullname = "Elly",
		.title = "The Theoretical Reaper",
		.description = "Species: Shinigami(?)\nArea of Study: Theoretical Physics / Forensic Pathology (dual-major)\n\nSlightly upset over being forgotten.\n\nHas apparently gotten herself a tutor in the “dark art” of theoretical physics. Her side-hustle is megalomania.\n\nLiterally on top of the world, without a care to spare for those beneath her. Try not to let too many stones crush the innocent yōkai below when her Tower of Babel starts to crumble.",
		.background = "stage6/spellbg_toe",
		.unlock = PBGM_STAGE6_BOSS1,
		.faces = FACES("normal", "eyes_closed", "angry", "shouting", "smug", "blush"),
	},
	[PROFILE_LOCKED] = {
		.name = "locked",
		.fullname = "Locked",
		.title = "...",
		.description = "You have not unlocked this character yet!",
		.unlock = PBGM_GAMEOVER,
		.background = "stage1/cirnobg",
		.faces = FACES("locked"),
	}
};

static int check_unlocked_profile(int i) {
	int selected = PROFILE_LOCKED;
	if(!profiles[i].unlock) {
		// for protagonists
		selected = i;
	} else if(profiles[i].unlock != PBGM_GAMEOVER) {
		if(progress_is_bgm_id_unlocked(profiles[i].unlock)) selected = i;
	}
	return selected;
}

static void charprofile_logic(MenuData *m) {
	dynarray_foreach(&m->entries, int i, MenuEntry *e, {
		e->drawdata += 0.05 * ((m->cursor != i) - e->drawdata);
	});
	MenuEntry *cursor_entry = dynarray_get_ptr(&m->entries, m->cursor);
	Font *font = res_font("small");
	char buf[512] = { 0 };
	int j = check_unlocked_profile(m->cursor);
	text_wrap(font, profiles[j].description, DESCRIPTION_WIDTH, buf, sizeof(buf));
	double height = text_height(font, buf, 0) + font_get_lineskip(font) * 2;

	fapproach_asymptotic_p(&m->drawdata[0], 1, 0.1, 1e-5);
	fapproach_asymptotic_p(&m->drawdata[1], 1 - cursor_entry->drawdata, 0.1, 1e-5);
	fapproach_asymptotic_p(&m->drawdata[2], height, 0.1, 1e-5);
}

static void charprofile_draw(MenuData *m) {
	assert(m->cursor < NUM_PROFILES);
	r_state_push();

	CharProfileContext *ctx = m->context;

	draw_main_menu_bg(m, SCREEN_W/4+100, 0, 0.1 * (0.5 + 0.5 * m->drawdata[1]), "menu/mainmenubg", profiles[m->cursor].background);
	draw_menu_title(m, "Character Profiles");


	// background behind description text
	float f = m->drawdata[0];
	float descbg_ofs = 100 + 30 * f - 20 * f - font_get_lineskip(res_font("small")) * 0.7;
	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W/4, SCREEN_H/3, 0);
	r_color4(0, 0, 0, 0.5);
	r_shader_standard_notex();
	r_mat_mv_translate(-120, descbg_ofs + m->drawdata[2] * 0.5, 0);
	r_mat_mv_scale(650, m->drawdata[2], 1);
	r_draw_quad();
	r_shader_standard();
	r_mat_mv_pop();

	CharProfiles i = ctx->char_draw_order[m->cursor];

	MenuEntry *e = dynarray_get_ptr(&m->entries, m->cursor);

	float o = 1 - e->drawdata*2;
	float pbrightness = 0.6 + 0.4 * o;

	float pofs = fmax(0.0f, e->drawdata * 1.5f - 0.5f);
	pofs = glm_ease_back_in(pofs);

	if(i != m->selected) {
		pofs = lerp(pofs, 1, menu_fade(m));
	}

	int selected = check_unlocked_profile(m->cursor);

	Color *color = RGBA(pbrightness, pbrightness, pbrightness, 1);

	// if not unlocked, darken the sprite so it's barely visible
	if(selected == PROFILE_LOCKED) {
		color = RGBA(0.0, 0.0, 0.0, 0.9);
	}

	Sprite *spr = e->arg;
	SpriteParams portrait_params = {
		.pos = { SCREEN_W/2 + 240 + 320 * pofs, SCREEN_H - spr->h * 0.5 },
		.sprite_ptr = spr,
		.shader = "sprite_default",
		.color = color,
	};

	r_draw_sprite(&portrait_params);

	if(selected != PROFILE_LOCKED) {
		portrait_params.sprite_ptr = portrait_get_face_sprite(profiles[selected].name, profiles[selected].faces[ctx->face]);
		r_draw_sprite(&portrait_params);
		r_mat_mv_push();
		text_draw_wrapped("Press [Fire] for alternate expressions", DESCRIPTION_WIDTH, &(TextParams) {
			.align = ALIGN_LEFT,
			.pos = { 25, 570 },
			.font = "standard",
			.shader = "text_default",
			.color = RGBA(0.9, 0.9, 0.9, 0.9),
		});
		r_mat_mv_pop();
	}

	r_mat_mv_push();
	r_mat_mv_translate(SCREEN_W/4, SCREEN_H/3, 0);
	r_mat_mv_push();
	r_mat_mv_push();

	if(e->drawdata != 0) {
		r_mat_mv_translate(0, -300 * e->drawdata, 0);
		r_mat_mv_rotate(M_PI * e->drawdata, 1, 0, 0);
	}

	text_draw(profiles[selected].fullname, &(TextParams) {
		.align = ALIGN_CENTER,
		.font = "big",
		.shader = "text_default",
		.color = RGBA(o, o, o, o),
	});
	r_mat_mv_pop();

	if(e->drawdata) {
		o = 1 - e->drawdata * 3;
	} else {
		o = 1;
	}

	text_draw(profiles[selected].title, &(TextParams) {
		.align = ALIGN_CENTER,
		.pos = { 20*(1-o), 30 },
		.shader = "text_default",
		.color = RGBA(o, o, o, o),
	});
	r_mat_mv_pop();

	text_draw_wrapped(profiles[selected].description, DESCRIPTION_WIDTH, &(TextParams) {
		.align = ALIGN_LEFT,
		.pos = { -175, 120 },
		.font = "small",
		.shader = "text_default",
		.color = RGBA(o, o, o, o),
	});
	r_mat_mv_pop();

	o = 0.3*sin(m->frames/20.0)+0.5;
	r_shader("sprite_default");

	r_draw_sprite(&(SpriteParams) {
		.sprite = "menu/arrow",
		.pos = { 30, SCREEN_H/3+10 },
		.color = RGBA(o, o, o, o),
		.scale = { 0.5, 0.7 },
	});

	r_draw_sprite(&(SpriteParams) {
		.sprite = "menu/arrow",
		.pos = { 30 + 340, SCREEN_H/3+10 },
		.color = RGBA(o, o, o, o),
		.scale = { 0.5, 0.7 },
		.flip.x = true,
	});

	r_state_pop();
}

static void action_show_character(MenuData *m, void *arg) {
	return;
}

static void add_character(MenuData *m, int i) {
	if(strcmp(profiles[i].name, "locked")) {
		log_debug("adding character: %s", profiles[i].name);
		Sprite *spr = portrait_get_base_sprite(profiles[i].name, NULL);
		MenuEntry *e = add_menu_entry(m, NULL, action_show_character, spr);
		e->transition = NULL;
	}
}

static void charprofile_free(MenuData *m) {
	free(m->context);
}

// TODO: add a better drawing animation for character selection
static bool charprofile_input_handler(SDL_Event *event, void *arg) {
	MenuData *m = arg;
	CharProfileContext *ctx = m->context;
	TaiseiEvent type = TAISEI_EVENT(event->type);

	if(type == TE_MENU_CURSOR_LEFT) {
		play_sfx_ui("generic_shot");
		m->cursor--;
		ctx->face = 0;
	} else if(type == TE_MENU_CURSOR_RIGHT) {
		play_sfx_ui("generic_shot");
		m->cursor++;
		ctx->face = 0;
	} else if(type == TE_MENU_ACCEPT) {
		if (check_unlocked_profile(m->cursor) != PROFILE_LOCKED) {
			play_sfx_ui("generic_shot");
			// show different expressions for selected character
			ctx->face++;
			if(!profiles[m->cursor].faces[ctx->face]) {
				ctx->face = 0;
			}
		}
	} else if(type == TE_MENU_ABORT) {
		play_sfx_ui("hit");
		close_menu(m);
	} else {
		return false;
	}

	m->cursor = (m->cursor % m->entries.num_elements) + m->entries.num_elements * (m->cursor < 0);

	return false;

}

void preload_charprofile_menu(void) {
	for(int i = 0; i < NUM_PROFILES-1; i++) {
		portrait_preload_base_sprite(profiles[i].name, NULL, RESF_PERMANENT);
		preload_resource(RES_TEXTURE, profiles[i].background, RESF_OPTIONAL);
	}
};

static void charprofile_input(MenuData *m) {
	events_poll((EventHandler[]){
		{ .proc = charprofile_input_handler, .arg = m },
		{ NULL }
	}, EFLAG_MENU);
}

MenuData *create_charprofile_menu(void) {
	MenuData *m = alloc_menu();

	preload_charprofile_menu();

	m->input = charprofile_input;
	m->draw = charprofile_draw;
	m->logic = charprofile_logic;
	m->end = charprofile_free;
	m->transition = TransFadeBlack;
	m->flags = MF_Abortable;

	CharProfileContext *ctx = calloc(1, sizeof(*ctx));
	ctx->prev_selected_char = -1;
	m->context = ctx;

	for(int i = 0; i < NUM_PROFILES; i++) {
		add_character(m, i);
	}

	for(CharProfiles c = 0; c < NUM_PROFILES; ++c) {
		ctx->char_draw_order[c] = c;
	}

	m->drawdata[1] = 1;

	return m;
}
