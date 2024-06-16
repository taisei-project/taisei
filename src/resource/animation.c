/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "animation.h"

#include "renderer/api.h"
#include "resource.h"
#include "util/kvparser.h"

static char *animation_path(const char *name) {
	return strjoin(ANI_PATH_PREFIX, name, ANI_EXTENSION, NULL);
}

static bool check_animation_path(const char *path) {
	return strendswith(path, ANI_EXTENSION);
}

// See ANIMATION_FORMAT.rst for a documentation of this syntax.
static bool animation_parse_sequence_spec(AniSequence **seq, int seq_capacity, const char *specstr) {
	const char *delaytoken = "d";
	const char *mirrortoken = "m";

	const char *p = specstr;

	// skip initial whitespace
	while(isspace(*p))
		p++;

	enum {
		ANISEQ_SPRITEIDX,
		ANISEQ_DELAY,
		ANISEQ_MIRROR
	};

	int delay = 1; // standard frame duration: one frame.
	bool mirrored = false;

	int command = 1;
	// This loop is supposed to parse one command per iteration.
	while(*p != '\0') {
		int state = ANISEQ_SPRITEIDX;
		if(strstartswith(p,delaytoken)) {
			p += strlen(delaytoken);
			state = ANISEQ_DELAY;
		} else if(strstartswith(p,mirrortoken)) {
			p += strlen(mirrortoken);
			state = ANISEQ_MIRROR;
		} else if(!isdigit(*p)) {
			log_error("AniSequence syntax error: '%s'[%d] does not start with a proper command",specstr,command);
			return false;
		}

		const char *endptr = p;
		int arg	= 0;
		if(!isspace(*p)) { // strtol will ignore leadingwhitespace and therefore eat the next sprite number if there is no arg!
			char *tmp;
			arg = strtol(p,&tmp,10);
			endptr = tmp;
		}

		bool no_arg = endptr-p == 0;

		if(*endptr != '\0' && !isspace(*endptr)) {
			log_error("AniSequence syntax error: '%s'[%d] contains unexpected character '%c'",specstr,command,*endptr);
			return false;
		}

		if(state != ANISEQ_MIRROR && no_arg) {
			log_error("AniSequence syntax error: '%s'[%d] expected argument after command",specstr,command);
			return false;
		}


		p = endptr;
		while(isspace(*p))
			p++;

		// expression parsed, now react

		if(state == ANISEQ_DELAY) {
			if(arg < 0) {
				log_error("AniSequence syntax error: '%s'[%d] delay cannot be negative.",specstr,command);
				return false;
			}
			delay = arg;
		} else if(state == ANISEQ_MIRROR) {
			if(no_arg) {
				mirrored = !mirrored;
			} else if(arg == 0 || arg == 1) {
				mirrored = arg;
			} else {
				log_error("AniSequence syntax error: '%s'[%d] mirror can only take 0 or 1 or nothing as an argument. %s",specstr,command,endptr);
				return false;
			}
		} else if(state == ANISEQ_SPRITEIDX) {
			if(arg < 0) {
				log_error("AniSequence syntax error: '%s'[%d] sprite index cannot be negative.",specstr,command);
				return false;
			}

			// temporarily store flipped sprites with negative indices.
			// a later pass will allocate actual flipped sprites for these and fix up the indices.
			int spriteidx = mirrored ? -(arg + 1) : arg;

			for(int i = 0; i < delay; i++) {
				if(seq_capacity <= (*seq)->length) {
					seq_capacity *= 2;
					*seq = mem_realloc(*seq, sizeof(AniSequence) + seq_capacity * sizeof(*(*seq)->frame_indices));
				}
				(*seq)->frame_indices[(*seq)->length++] = spriteidx;
			}
		}
		command++;
	}

	if((*seq)->length <= 0) {
		log_error("AniSequence syntax error: '%s' empty sequence.",specstr);
		return false;
	}

	return true;
}

static bool animation_parse_callback(const char *key, const char *value, void *data) {
	Animation *ani = (Animation *)data;

	// parse fixed keys

	if(!strcmp(key, "@sprite_count")) {
		char *endptr;
		ani->sprite_count = strtol(value,&endptr,10);
		if(*value == '\0' || *endptr != '\0' || ani->sprite_count <= 0) {
			log_error("%s must be a positive integer (got `%s`)", key, value);
			return false;
		}
		return true;
	}

	if(key[0] == '@') {
		log_warn("Unknown animation property `%s` ignored", key);
		return true;
	}

	// otherwise it is a sequence

	AniSequence *prev_seq;
	if((prev_seq = ht_get(&ani->sequences, key, NULL))) {
		log_warn("Animation sequence `%s` overwritten", key);
	}

	int init_seq_size = 4;
	AniSequence *seq = ALLOC_FLEX(typeof(*seq), init_seq_size * sizeof(*seq->frame_indices));
	if(!animation_parse_sequence_spec(&seq, init_seq_size, value)) {
		mem_free(seq);
		return false;
	}

	ht_set(&ani->sequences, key, seq);
	mem_free(prev_seq);
	return true;
}

static void *free_sequence_callback(const char *key, void *data, void *arg) {
	mem_free(data);
	return NULL;
}

static void load_animation_stage1(ResourceLoadState *st);
static void load_animation_stage2(ResourceLoadState *st);

static void load_animation_stage1(ResourceLoadState *st) {
	auto ani = ALLOC(Animation);
	ht_create(&ani->sequences);

	if(!parse_keyvalue_file_cb(st->path, animation_parse_callback, ani)) {
		ht_foreach(&ani->sequences, free_sequence_callback, NULL);
		ht_destroy(&ani->sequences);
		mem_free(ani);
		res_load_failed(st);
		return;
	}

	if(ani->sprite_count <= 0) {
		log_error("Animation sprite count of '%s', must be positive integer", st->name);
		ht_foreach(&ani->sequences, free_sequence_callback, NULL);
		ht_destroy(&ani->sequences);
		mem_free(ani);
		res_load_failed(st);
		return;
	}

	char buf[strlen(st->name) + sizeof(".frame0000")];

	for(int i = 0; i < ani->sprite_count; ++i) {
		snprintf(buf, sizeof(buf), "%s.frame%04d", st->name, i);
		res_load_dependency(st, RES_SPRITE, buf);
	}

	res_load_continue_after_dependencies(st, load_animation_stage2, ani);
}

struct anim_remap_state {
	Animation *ani;
	ht_int2int_t flip_map;
	int num_flipped_sprites;
	int errors;
};

static void flip_sprite(Sprite *s) {
	FloatRect *tex_area = &s->tex_area;
	tex_area->x += tex_area->w;
	tex_area->w *= -1;
}

static int remap_frame_index(struct anim_remap_state *st, int idx) {
	int64_t remapped_idx;

	if(idx >= 0) {
		return idx;
	}

	if(st->num_flipped_sprites > 0 && ht_lookup(&st->flip_map, idx, &remapped_idx)) {
		return remapped_idx;
	}

	if(st->num_flipped_sprites == 0) {
		ht_create(&st->flip_map);
	}

	remapped_idx = st->ani->sprite_count;
	int local_idx = st->num_flipped_sprites;
	int orig_idx = -(idx + 1);

	++st->num_flipped_sprites;
	++st->ani->sprite_count;

	ht_set(&st->flip_map, idx, remapped_idx);

	st->ani->local_sprites = mem_realloc(st->ani->local_sprites, sizeof(*st->ani->local_sprites) * st->num_flipped_sprites);
	st->ani->local_sprites[local_idx] = *st->ani->sprites[orig_idx];
	flip_sprite(st->ani->local_sprites + local_idx);

	log_debug("%i -> %i", orig_idx, (int)remapped_idx);
	log_debug("sprite_count: %i", st->ani->sprite_count);

	return remapped_idx;
}

static void *remap_sequence_callback(const char *key, void *value, void *varg) {
	AniSequence *seq = value;
	struct anim_remap_state *st = varg;
	// needs to be cached, because we may grow the count to allocate flipped sprites
	int sprite_count = st->ani->sprite_count;
	int errors = 0;

	for(int i = 0; i < seq->length; i++) {
		int abs_idx = seq->frame_indices[i];
		abs_idx = abs_idx >= 0 ? abs_idx : -(abs_idx + 1);

		if(abs_idx >= sprite_count) {
			log_error("Animation sequence `%s`: Sprite index %i is higher than sprite_count (%i).", key, abs_idx, sprite_count);
			errors++;
		}

		if(seq->frame_indices[i] < 0) {
			seq->frame_indices[i] = remap_frame_index(st, seq->frame_indices[i]);
		}
	}

	if(errors) {
		st->errors += errors;
		return st;
	}

	return NULL;
}

static void unload_animation(void *vani) {
	Animation *ani = vani;
	ht_foreach(&ani->sequences, free_sequence_callback, NULL);
	ht_destroy(&ani->sequences);
	mem_free(ani->sprites);
	mem_free(ani->local_sprites);
	mem_free(ani);
}

static void load_animation_stage2(ResourceLoadState *st) {
	Animation *ani = NOT_NULL(st->opaque);
	ani->sprites = ALLOC_ARRAY(ani->sprite_count, typeof(*ani->sprites));

	char buf[strlen(st->name) + sizeof(".frame0000")];
	struct anim_remap_state remap_state = { 0 };

	for(int i = 0; i < ani->sprite_count; ++i) {
		snprintf(buf, sizeof(buf), "%s.frame%04d", st->name, i);

		if(!(ani->sprites[i] = res_get_data(RES_SPRITE, buf, st->flags))) {
			log_error("Animation frame '%s' not found but @sprite_count was %d",buf,ani->sprite_count);
			unload_animation(ani);
			ani = NULL;
			goto done;
		}
	}

	remap_state.ani = ani;
	int prev_sprite_count = ani->sprite_count;

	if(ht_foreach(&ani->sequences, remap_sequence_callback, &remap_state) != NULL) {
		unload_animation(ani);
		ani = NULL;
	}

	if(ani) {
		if(ani->sprite_count != prev_sprite_count) {
			// remapping generated new flipped sprites - add them to our sprites array

			assume(ani->sprite_count > prev_sprite_count);
			assume(remap_state.num_flipped_sprites == ani->sprite_count - prev_sprite_count);
			assume(ani->local_sprites != NULL);
			ani->sprites = mem_realloc(ani->sprites, sizeof(*ani->sprites) * ani->sprite_count);

			for(int i = 0; i < remap_state.num_flipped_sprites; ++i) {
				ani->sprites[prev_sprite_count + i] = ani->local_sprites + i;
			}
		} else {
			assert(remap_state.num_flipped_sprites == 0);
			assert(ani->local_sprites == NULL);
		}
	}

done:
	if(remap_state.num_flipped_sprites > 0) {
		ht_destroy(&remap_state.flip_map);
	}

	if(ani) {
		res_load_finished(st, ani);
	} else {
		res_load_failed(st);
	}
}

AniSequence *get_ani_sequence(Animation *ani, const char *seqname) {
	AniSequence *seq = ht_get(&ani->sequences, seqname, NULL);

	if(seq == NULL) {
		log_fatal("Sequence '%s' not found.", seqname);
	}

	return seq;
}

Sprite *animation_get_frame(Animation *ani, AniSequence *seq, int seqframe) {
	int idx = seq->frame_indices[seqframe % seq->length];
	assert(idx < ani->sprite_count);
	return ani->sprites[idx];
}

ResourceHandler animation_res_handler = {
	.type = RES_ANIM,
	.typename = "animation",
	.subdir = ANI_PATH_PREFIX,

	.procs = {
		.find = animation_path,
		.check = check_animation_path,
		.load = load_animation_stage1,
		.unload = unload_animation,
	},
};
