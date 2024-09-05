/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stageinfo.h"
#include "stages/stages.h"

#include "dynstage.h"

static struct {
	DYNAMIC_ARRAY(StageInfo) stages;
	StageProgress **stages_progress;
} stageinfo;

static void add_stage(
	uint16_t id,
	StageProcs *procs,
	StageType type,
	const char *title,
	const char *subtitle,
	AttackInfo *spell,
	Difficulty diff
) {
	dynarray_append(&stageinfo.stages, {
		.id = id,
		.procs = procs,
		.type = type,
		.spell = spell,
		.difficulty = diff,
		.title = title ? mem_strdup(title) : NULL,
		.subtitle = subtitle ? mem_strdup(subtitle) : NULL,
	});
}

static void add_spellpractice_stage(
	StageInfo *s,
	AttackInfo *a,
	int *spellnum,
	uint16_t spellbits,
	Difficulty diff
) {
	uint16_t id = spellbits | a->idmap[diff - D_Easy] | (s->id << 8);

	char *title = strfmt("Spell %d", ++(*spellnum));
	char *subtitle = strjoin(a->name, " ~ ", difficulty_name(diff), NULL);

	add_stage(id, s->procs->spellpractice_procs, STAGE_SPELL, title, subtitle, a, diff);

	mem_free(title);
	mem_free(subtitle);
}

static void add_spellpractice_stages(
	int *spellnum,
	bool (*filter)(AttackInfo*),
	uint16_t spellbits
) {
	for(int i = 0 ;; ++i) {
		StageInfo *s = dynarray_get_ptr(&stageinfo.stages, i);

		if(s->type == STAGE_SPELL || !s->spell) {
			break;
		}

		for(AttackInfo *a = s->spell; a->name; ++a) {
			if(!filter(a)) {
				continue;
			}

			for(Difficulty diff = D_Easy; diff < D_Easy + NUM_SELECTABLE_DIFFICULTIES; ++diff) {
				if(a->idmap[diff - D_Easy] >= 0) {
					add_spellpractice_stage(s, a, spellnum, spellbits, diff);

					// stages may have gotten realloc'd, so we must update the pointer
					s = dynarray_get_ptr(&stageinfo.stages, i);
				}
			}
		}
	}
}

static bool spellfilter_normal(AttackInfo *spell) {
	return spell->type != AT_ExtraSpell;
}

static bool spellfilter_extra(AttackInfo *spell) {
	return spell->type == AT_ExtraSpell;
}

static void stageinfo_fill(StagesExports *e);

void stageinfo_init(void) {
	dynstage_init();
	stageinfo_fill(dynstage_get_exports());
	stageinfo.stages_progress = ALLOC_ARRAY(
		stageinfo.stages.num_elements, typeof(*stageinfo.stages_progress));
}

void stageinfo_reload(void) {
	if(!dynstage_reload_library()) {
		log_debug("dynstage not reloaded");
		return;
	}

	attr_unused StageInfo *prev_addr = stageinfo.stages.data;
	attr_unused uint prev_count = stageinfo.stages.num_elements;

	dynarray_foreach_elem(&stageinfo.stages, StageInfo *stg, {
		mem_free(stg->title);
		mem_free(stg->subtitle);
	});

	stageinfo.stages.num_elements = 0;
	stageinfo_fill(dynstage_get_exports());

	assert(stageinfo.stages.data == prev_addr);
	assert(stageinfo.stages.num_elements == prev_count);

	log_debug("Stageinfo updated after dynstage reload");
}

static void stageinfo_fill(StagesExports *e) {
	int spellnum = 0;

//	         id  procs            type           title          subtitle                       spells                       diff
	add_stage(1, e->stage1.procs, STAGE_STORY,   "Stage 1",     "Misty Lake Encounter",        e->stage1.spells, D_Any);
	add_stage(2, e->stage2.procs, STAGE_STORY,   "Stage 2",     "Riverside Hina-Nagashi",      e->stage2.spells, D_Any);
	add_stage(3, e->stage3.procs, STAGE_STORY,   "Stage 3",     "Crawly Mountain Ascent",      e->stage3.spells, D_Any);
	add_stage(4, e->stage4.procs, STAGE_STORY,   "Stage 4",     "Forgotten Mansion",           e->stage4.spells, D_Any);
	add_stage(5, e->stage5.procs, STAGE_STORY,   "Stage 5",     "Climbing the Tower of Babel", e->stage5.spells, D_Any);
	add_stage(6, e->stage6.procs, STAGE_STORY,   "Stage 6",     "Roof of the World",           e->stage6.spells, D_Any);
	add_stage(7, e->stagex.procs, STAGE_SPECIAL, "Extra Stage", "Descent into Madness",        e->stagex.spells, D_Extra);

#ifdef TAISEI_BUILDCONF_TESTING_STAGES
	add_stage(0x40|0, e->testing.dps_single, STAGE_SPECIAL, "DPS Test", "Single target",    NULL, D_Normal);
	add_stage(0x40|1, e->testing.dps_multi,  STAGE_SPECIAL, "DPS Test", "Multiple targets", NULL, D_Normal);
	add_stage(0x40|2, e->testing.dps_boss,   STAGE_SPECIAL, "DPS Test", "Boss",             NULL, D_Normal);
#endif

	// generate spellpractice stages
	add_spellpractice_stages(&spellnum, spellfilter_normal, STAGE_SPELL_BIT);
	add_spellpractice_stages(&spellnum, spellfilter_extra, STAGE_SPELL_BIT | STAGE_EXTRASPELL_BIT);

#ifdef TAISEI_BUILDCONF_TESTING_STAGES
	add_spellpractice_stage(
		dynarray_get_ptr(&stageinfo.stages, 0),
		e->testing.benchmark_spell, &spellnum, STAGE_SPELL_BIT, D_Extra
	);
#endif

	dynarray_compact(&stageinfo.stages);

#ifdef DEBUG
	dynarray_foreach(&stageinfo.stages, int i, StageInfo *stg, {
		if(stg->type == STAGE_SPELL && !(stg->id & STAGE_SPELL_BIT)) {
			log_fatal("Spell stage has an ID without the spell bit set: 0x%04x", stg->id);
		}

		dynarray_foreach(&stageinfo.stages, int j, StageInfo *stg2, {
			if(stg != stg2 && stg->id == stg2->id) {
				log_fatal("Duplicate ID %X in stages array, indices: %i, %i", stg->id, i, j);
			}
		});
	});
#endif
}

void stageinfo_shutdown(void) {
	dynarray_foreach(&stageinfo.stages, int i, StageInfo *stg, {
		mem_free(stg->title);
		mem_free(stg->subtitle);
		mem_free(stageinfo.stages_progress[i]);
	});

	dynarray_free_data(&stageinfo.stages);
	mem_free(stageinfo.stages_progress);
	dynstage_shutdown();
}

size_t stageinfo_get_num_stages(void) {
	return stageinfo.stages.num_elements;
}

StageInfo *stageinfo_get_by_index(size_t index) {
	return dynarray_get_ptr(&stageinfo.stages, index);
}

StageInfo *stageinfo_get_by_id(uint16_t n) {
	// TODO speed this up (use a hashtable maybe)

	dynarray_foreach_elem(&stageinfo.stages, StageInfo *stg, {
		if(stg->id == n) {
			return stg;
		}
	});

	return NULL;
}

StageInfo *stageinfo_get_by_spellcard(AttackInfo *spell, Difficulty diff) {
	if(!spell) {
		return NULL;
	}

	dynarray_foreach_elem(&stageinfo.stages, StageInfo *stg, {
		if(stg->spell == spell && stg->difficulty == diff) {
			return stg;
		}
	});

	return NULL;
}

StageProgress *stageinfo_get_progress(StageInfo *stage, Difficulty diff, bool allocate) {
	// D_Any stages will have a separate StageProgress for every selectable difficulty.
	// Stages with a fixed difficulty setting (spellpractice, extra stage...) obviously get just one and the diff parameter is ignored.

	// This stuff must stay around until progress_save(), which happens on shutdown.
	// So do NOT try to free any pointers this function returns, that will fuck everything up.

	uint idx = dynarray_indexof(&stageinfo.stages, stage);
	StageProgress **prog = stageinfo.stages_progress + idx;
	bool fixed_diff = (stage->difficulty != D_Any);

	if(!fixed_diff && (diff < D_Easy || diff > D_Lunatic)) {
		return NULL;
	}

	if(!*prog) {
		if(!allocate) {
			return NULL;
		}

		*prog = ALLOC_ARRAY(fixed_diff ? 1 : NUM_SELECTABLE_DIFFICULTIES, typeof(**prog));
	}

	return *prog + (fixed_diff ? 0 : diff - D_Easy);
}

StageProgress *stageinfo_get_progress_by_id(uint16_t id, Difficulty diff, bool allocate) {
	StageInfo *info = stageinfo_get_by_id(id);

	if(info != NULL) {
		return stageinfo_get_progress(info, diff, allocate);
	}

	return NULL;
}
