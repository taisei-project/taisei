/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "animation.h"
#include "texture.h"
#include "resource.h"
#include "list.h"
#include "renderer/api.h"

static char* animation_path(const char *name) {
	return strjoin(RES_PATHPREFIX_ANIMATION, name, ANI_EXTENSION, NULL);
}

static bool check_animation_path(const char *path) {
	return strendswith(path, ANI_EXTENSION);
}

// See ANIMATION_FORMAT.rst for a documentation of this syntax.
static bool animation_parse_sequence_spec(AniSequence *seq, const char *specstr) {
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
	int seqcapacity = 0;

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
			int spriteidx = arg;
			for(int i = 0; i < delay; i++) {
				if(seqcapacity <= seq->length) {
					seqcapacity++;
					seqcapacity *= 2;
					seq->frames = realloc(seq->frames,sizeof(AniSequenceFrame)*seqcapacity);
				}
				seq->frames[seq->length].spriteidx = spriteidx;
				seq->frames[seq->length].mirrored = mirrored;
				seq->length++;
			}
		}
		command++;
	}

	if(seq->length <= 0) {
		log_error("AniSequence syntax error: '%s' empty sequence.",specstr);
		return false;
	}

	return true;
}

static bool animation_parse_callback(const char *key, const char *value, void *data) {
	Animation *ani = (Animation *)data;

	// parse fixed keys

	if(!strcmp(key,"@sprite_count")) {
		char *endptr;
		ani->sprite_count = strtol(value,&endptr,10);
		if(*value == '\0' || *endptr != '\0' || ani->sprite_count <= 0) {
			log_error("Syntax error: %s must be positive integer",key);
			return false;
		}
		return true;
	}

	// otherwise it is a sequence

	AniSequence *seq = calloc(1,sizeof(AniSequence));
	bool rc = animation_parse_sequence_spec(seq, value);
	if(!rc) {
		free(seq->frames);
		free(seq);
		return false;
	}

	ht_set(&ani->sequences, key, seq);
	return true;
}

static void *free_sequence_callback(const char *key, void *data, void *arg) {
	AniSequence *seq = data;
	free(seq->frames);
	free(seq);

	return NULL;
}

static void *load_animation_begin(ResourceLoadInfo rli) {
	Animation *ani = calloc(1, sizeof(Animation));
	ht_create(&ani->sequences);

	if(!parse_keyvalue_file_cb(rli.path, animation_parse_callback, ani)) {
		ht_foreach(&ani->sequences, free_sequence_callback, NULL);
		ht_destroy(&ani->sequences);
		free(ani);
		return NULL;
	}

	if(ani->sprite_count <= 0) {
		log_error("Animation sprite count of '%i', must be positive integer", ani->sprite_count);
		ht_foreach(&ani->sequences, free_sequence_callback, NULL);
		ht_destroy(&ani->sequences);
		free(ani);
		return NULL;
	}

	ani->sprites = calloc(ani->sprite_count, sizeof(*ani->sprites));
	char buf[strlen(rli.name) + sizeof(".frame0000")];

	for(int i = 0; i < ani->sprite_count; ++i) {
		snprintf(buf, sizeof(buf), "%s.frame%04d", rli.name, i);
		ani->sprites[i] = res_ref(RES_SPRITE, buf, rli.flags);
	}

	return ani;
}

union check_sequence_frame_callback_arg {
	int sprite_count, errors;
};

static void *check_sequence_frame_callback(const char *key, void *value, void *varg) {
	AniSequence *seq = value;
	union check_sequence_frame_callback_arg *arg = varg;
	int sprite_count = arg->sprite_count;
	int errors = 0;

	for(int i = 0; i < seq->length; i++) {
		if(seq->frames[i].spriteidx >= sprite_count) {
			log_error("Animation sequence %s: Sprite index %d is higher than sprite_count.", key, seq->frames[i].spriteidx);
			errors++;
		}
	}

	if(errors) {
		arg->errors = errors;
		return arg;
	}

	return NULL;
}

static void unload_animation(void *vani) {
	Animation *ani = vani;

	if(ani->sprites) {
		res_unref(ani->sprites, ani->sprite_count);
		free(ani->sprites);
	}

	ht_foreach(&ani->sequences, free_sequence_callback, NULL);
	ht_destroy(&ani->sequences);
	free(ani);
}

attr_nonnull(2)
static void* load_animation_end(ResourceLoadInfo rli, void *opaque) {
	Animation *ani = opaque;
	union check_sequence_frame_callback_arg arg = { ani->sprite_count };

	if(ht_foreach(&ani->sequences, check_sequence_frame_callback, &arg) != NULL) {
		unload_animation(ani);
		return NULL;
	}

	for(int i = 0; i < ani->sprite_count; ++i) {
		if(res_ref_data(ani->sprites[i]) == NULL) {
			log_error("Animation frame %i failed to load", i);
			unload_animation(ani);
			return NULL;
		}
	}

	return ani;
}

Animation *get_ani(const char *name) {
	return res_get_data(RES_ANIMATION, name, RESF_DEFAULT);
}

AniSequence *get_ani_sequence(Animation *ani, const char *seqname) {
	AniSequence *seq = ht_get(&ani->sequences, seqname, NULL);

	if(seq == NULL) {
		log_fatal("Sequence '%s' not found.",seqname);
	}

	return seq;
}

Sprite* animation_get_frame(Animation *ani, AniSequence *seq, int seqframe) {
	AniSequenceFrame *f = &seq->frames[seqframe%seq->length];
	if(f->mirrored) {
		memcpy(&ani->transformed_sprite, res_ref_data(ani->sprites[f->spriteidx]), sizeof(Sprite));
		// XXX: maybe add sprite_flip() or something
		FloatRect *tex_area = &ani->transformed_sprite.tex_area;
		tex_area->x += tex_area->w;
		tex_area->w *= -1;
		return &ani->transformed_sprite;
	}

	assert(f->spriteidx < ani->sprite_count);
	return res_ref_data(ani->sprites[f->spriteidx]);
}

ResourceHandler animation_res_handler = {
	.type = RES_ANIMATION,

	.procs = {
		.find = animation_path,
		.check = check_animation_path,
		.begin_load = load_animation_begin,
		.end_load = load_animation_end,
		.unload = unload_animation,
	},
};
