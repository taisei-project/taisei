/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "menu.h"
#include "portrait.h"

#define NUM_PROFILES 11
#define LOCKED_PROFILE NUM_PROFILES-1
#define DESCRIPTION_WIDTH (SCREEN_W / 3 + 80)

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
} CharProfiles;

typedef struct CharacterProfile {
	const char *name;
	const char *fullname;
	const char *title;
	const char *description;
	const char *background;
	const char *unlock;
	Sprite *sprite;
	char *faces[11];
} CharacterProfile;

typedef struct CharProfileContext {
	int8_t char_draw_order[NUM_PROFILES];
	int8_t prev_selected_char;
	int8_t face;
} CharProfileContext;

static struct CharacterProfile profiles[NUM_PROFILES] = {
	[PROFILE_REIMU] = {
		.name = "reimu",
		.fullname = "Hakurei Reimu",
		.title = "Shrine Maiden of Fantasy",
		.description = "Species: Human\nArea of Study: Literature (Fiction)\n\nThe incredibly particular shrine maiden.\n\nShe’s taking a break from her busy novel-reading schedule to take care of an incident.\n\nMostly, she has a vague feeling of sentimentality for a bygone era.\n\nRegardless, when the residents of Yōkai Mountain show up sobbing at her door, she has no choice but to put her book down and spring into action.",
		.background = "reimubg",
		.faces = {"normal", "surprised", "unamused", "happy", "sigh", "smug", "puzzled", "assertive", "irritated", "outraged"},
	},
	[PROFILE_MARISA] = {
		.name = "marisa",
		.fullname = "Kirisame Marisa",
		.title = "Unbelievably Ordinary Magician",
		.description = "Species: Human\nArea of Study: Eclecticism\n\nThe confident, hyperactive witch.\n\nCurious as ever about the limits of magic and the nature of reality, and when she hears about “eldritch lunacy,” her curiosity gets the better of her.\n\nBut maybe this “grimoire” is best left unread?\n\nNot that a warning like that would stop her anyways. So, off she goes.",
		.background = "marisa_bombbg",
		.faces = {"normal", "surprised", "sweat_smile", "happy", "smug", "puzzled", "unamused", "inquisitive"},
	},
	[PROFILE_YOUMU] = {
		.name = "youmu",
		.fullname = "Konpaku Yōmu",
		.title = "Swordswoman Between Worlds",
		.description = "Species: Half-Human, Half-Phantom\nArea of Study: Fencing\n\nThe swordswoman of the somewhat-dead.\n\n“While you were playing games, I was studying the blade” … is probably what she’d say.\n\nEntirely too serious about everything, and utterly terrified of (half of) her own existence.\n\nStill, she’s been given an important mission by Lady Yuyuko. Hopefully she doesn’t let it get to her head.",
		.background = "youmu_bombbg1",
		.faces = {"normal", "eeeeh", "embarrassed", "eyes_closed", "chuuni", "happy", "relaxed", "sigh", "smug", "surprised", "unamused"},
	},
	[PROFILE_CIRNO] = {
		.name = "cirno",
		.fullname = "Cirno",
		.title = "Thermodynamic Ice Fairy",
		.description = "Species: Fairy\nField of Study: Thermodynamic Systems\nThe lovably-foolish ice fairy.\n\nPerhaps she’s a bit dissatisfied after her recent duels with secret gods and hellish fairies?\n\nConsider going easy on her.",
		.background = "stage1/cirnobg",
		.unlock = "stage1boss",
		.faces = {"normal", "angry", "defeated"},
	},
	[PROFILE_HINA] = {
		.name = "hina",
		.fullname = "Hina",
		.title = "Gyroscopic Pestilence God",
		.description = "Species: Pestilence God\nField of Study: Gyroscopic Stabilization\n\nGuardian of Yōkai Mountain. Her angular momentum is out of this world!\n\nShe seems awfully concerned with your health and safety, to the point of being quite overbearing.\n\nYou’re old enough to decide your own path in life, though.",
		.background = "stage2/spellbg2",
		.unlock = "stage2boss",
		.faces = {"normal", "concerned", "serious", "defeated"},
	},
	[PROFILE_SCUTTLE] = {
		.name = "scuttle",
		.fullname = "Scuttle",
		.title = "???",
		.description = "????",
		.unlock = "stage3boss",
		.background = "stage3/spellbg1",
		.faces = {"normal"},
	},
	[PROFILE_WRIGGLE] = {
		.name = "wriggle",
		.fullname = "Wriggle Nightbug",
		.title = "Insect Rights Activist",
		.description = "Species: Insect\nField of Study: Evolutionary Socio-Entomology\n\nA bright bug - or was it insect? - far from her usual stomping grounds.\nShe feels that Insects have lost their “rightful place” in history.\nIf she thinks she has a raw evolutionary deal, someone ought to tell her about the trilobites…",
		.unlock = "stage3boss",
		.background = "stage3/spellbg2",
		.faces = {"normal", "calm", "outraged", "outraged_unlit", "proud", "defeated"},
	},
	[PROFILE_KURUMI] = {
		.name = "kurumi",
		.fullname = "Kurumi",
		.title = "High-Society Phlebotomist",
		.description = "Species: Vampire\nField of Study: Phlebotomy\n\nVampiric blast from the past. Doesn’t she know Gensōkyō already has new bloodsuckers in town?\n\nSucking blood is what she’s good at, but her current job is a gatekeeper, and her true passion is high fashion.\n\nEveryone’s got multiple side-hustles these days…",
		.unlock = "stage4boss",
		.background = "stage4/kurumibg1",
		.faces = {"normal", "dissatisfied", "puzzled", "tsun", "tsun_blush", "defeated"},
	},
	[PROFILE_IKU] = {
		.name = "iku",
		.fullname = "Iku",
		.title = "Fulminologist of the Heavens",
		.description = "Species: Oarfish\nField of Study: Meteorology (Fulminology)\n\nMessenger from the clouds, and an amateur meteorologist.\n\nSeems to have been reading a few too many outdated psychiatry textbooks in recent times.\n\nDespite being so formal and stuffy, she seems to be the only one who knows what’s going on.",
		.background = "stage5/spell_lightning",
		.unlock = "stage5boss",
		.faces = {"normal", "smile", "serious", "eyes_closed", "defeated"},
	},
	[PROFILE_ELLY] = {
		.name = "elly",
		.fullname = "Elly",
		.title = "The Theoretical Reaper",
		.description = "Species: Shinigami(?)\nField of Study: Theoretical Physics / Forensic Pathology (dual-major)\n\nSlightly upset over being forgotten.\n\nHas apparently gotten herself a tutor in the “dark art” of theoretical physics. Her side-hustle is megalomania.\n\nLiterally on top of the world, without a care to spare for those beneath her. Try not to let too many stones crush the innocent yōkai below when her Tower of Babel starts to crumble.",
		.background = "stage6/spellbg_toe",
		.unlock = "stage6boss_phase1",
		.faces = {"normal", "eyes_closed", "angry", "shouting", "smug", "blush"},
	},
	[PROFILE_LOCKED] = {
		.name = "locked",
		.fullname = "Locked",
		.title = "...",
		.description = "You have not unlocked this character yet!",
		.background = "stage1/cirnobg",
		.faces = {"locked"},
		.unlock = "lockedboss",
	}
};

MenuData *create_charprofile_menu(void);
