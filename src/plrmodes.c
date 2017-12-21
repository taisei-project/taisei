/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "player.h"
#include "global.h"
#include "stage.h"

#include "plrmodes.h"
#include "plrmodes/marisa.h"
#include "plrmodes/youmu.h"
#include "plrmodes/reimu.h"

static PlayerCharacter *player_characters[] = {
	&character_marisa,
	&character_youmu,
	&character_reimu,
};

static PlayerMode *player_modes[] = {
	&plrmode_marisa_a,
	&plrmode_marisa_b,
	&plrmode_youmu_a,
	&plrmode_youmu_b,
	&plrmode_reimu_a,
	&plrmode_reimu_b,
};

PlayerCharacter* plrchar_get(CharacterID id) {
	assert((unsigned)id < NUM_CHARACTERS);
	PlayerCharacter *pc = player_characters[id];
	assert(pc->id == id);
	return pc;
}

void plrchar_preload(PlayerCharacter *pc) {
	preload_resource(RES_ANIM, pc->player_sprite_name, RESF_DEFAULT);
	preload_resource(RES_TEXTURE, pc->dialog_sprite_name, RESF_DEFAULT);
}

int plrmode_repr(char *out, size_t outsize, PlayerMode *mode) {
	assert(mode->character != NULL);
	assert((unsigned)mode->shot_mode < NUM_SHOT_MODES_PER_CHARACTER);

	return snprintf(out, outsize, "%s%c",
		mode->character->lower_name,
		mode->shot_mode + 'A'
	);
}

PlayerMode* plrmode_find(CharacterID char_id, ShotModeID shot_id) {
	for(int i = 0; i < NUM_PLAYER_MODES; ++i) {
		PlayerMode *mode = player_modes[i];

		if(mode->character->id == char_id && mode->shot_mode == shot_id) {
			return mode;
		}
	}

	return NULL;
}

PlayerMode* plrmode_parse(const char *name) {
	CharacterID char_id = (CharacterID)-1;
	ShotModeID shot_id = (ShotModeID)-1;
	char buf[strlen(name) + 1];
	strcpy(buf, name);

	for(int i = 0; i < sizeof(buf); ++i) {
		buf[i] = tolower(buf[i]);
	}

	if(!*buf) {
		log_debug("Got an empty string");
		return NULL;
	}

	char shot = buf[sizeof(buf) - 2];

	if(shot < 'a' || shot >= 'a' + NUM_SHOT_MODES_PER_CHARACTER) {
		log_debug("Got invalid shotmode: %c", shot);
		return NULL;
	}

	shot_id = shot - 'a';
	buf[sizeof(buf) - 2] = 0;

	for(int i = 0; i < NUM_CHARACTERS; ++i) {
		PlayerCharacter *chr = player_characters[i];

		if(!strcmp(buf, chr->lower_name)) {
			char_id = chr->id;
		}
	}

	if(char_id == (CharacterID)-1) {
		log_debug("Got invalid character: %s", buf);
		return NULL;
	}

	return plrmode_find(char_id, shot_id);
}

void plrmode_preload(PlayerMode *mode) {
	assert(mode != NULL);
	plrchar_preload(mode->character);

	if(mode->procs.preload) {
		mode->procs.preload();
	}
}
