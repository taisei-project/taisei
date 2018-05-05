/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "animation.h"
#include "texture.h"
#include "resource.h"
#include "list.h"
#include "renderer/api.h"

ResourceHandler animation_res_handler = {
	.type = RES_ANIM,
	.typename = "animation",
	.subdir = ANI_PATH_PREFIX,

	.procs = {
		.find = animation_path,
		.check = check_animation_path,
		.begin_load = load_animation_begin,
		.end_load = load_animation_end,
		.unload = unload_animation,
	},
};

char* animation_path(const char *name) {
	return strjoin(ANI_PATH_PREFIX, name, ANI_EXTENSION, NULL);
}

bool check_animation_path(const char *path) {
	return strendswith(path, ANI_EXTENSION);
}

typedef struct AnimationLoadData {
	Animation *ani;
	char *basename;
} AnimationLoadData;

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
			log_warn("AniSequence syntax error: '%s'[%d] does not start with a proper command",specstr,command);
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
			log_warn("AniSequence syntax error: '%s'[%d] contains unexpected character '%c'",specstr,command,*endptr);
			return false;
		}

		if(state != ANISEQ_MIRROR && no_arg) {
			log_warn("AniSequence syntax error: '%s'[%d] expected argument after command",specstr,command);
			return false;
		}


		p = endptr;
		while(isspace(*p))
			p++;

		// expression parsed, now react

		if(state == ANISEQ_DELAY) {
			if(arg < 0) {
				log_warn("AniSequence syntax error: '%s'[%d] delay cannot be negative.",specstr,command);
				return false;
			}
			delay = arg;
		} else if(state == ANISEQ_MIRROR) {
			if(no_arg) {
				mirrored = !mirrored;
			} else if(arg == 0 || arg == 1) {
				mirrored = arg;
			} else {
				log_warn("AniSequence syntax error: '%s'[%d] mirror can only take 0 or 1 or nothing as an argument. %s",specstr,command,endptr);
				return false;
			}
		} else if(state == ANISEQ_SPRITEIDX) {
			if(arg < 0) {
				log_warn("AniSequence syntax error: '%s'[%d] sprite index cannot be negative.",specstr,command);
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
		log_warn("AniSequence syntax error: '%s' empty sequence.",specstr);
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
			log_warn("Syntax error: %s must be positive integer",key);
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

	hashtable_set_string(ani->sequences,key,seq);
	return true;
}

static void *free_sequence_callback(void *key, void *data, void *arg) {
	AniSequence *seq = (AniSequence *)data;
	free(seq->frames);
	free(seq);

	return NULL;
}

void* load_animation_begin(const char *filename, uint flags) {
	char *basename = resource_util_basename(ANI_PATH_PREFIX, filename);
	char name[strlen(basename) + 1];
	strcpy(name, basename);

	Animation *ani = calloc(1, sizeof(Animation));
	ani->sequences = hashtable_new_stringkeys();

	if(!parse_keyvalue_file_cb(filename, animation_parse_callback, ani)) {
		hashtable_foreach(ani->sequences, free_sequence_callback, NULL);
		hashtable_free(ani->sequences);
		free(ani);
		free(basename);
		return NULL;
	}

	if(ani->sprite_count <= 0) {
		log_warn("Animation sprite count of '%s' must be positive integer?", name);
		hashtable_foreach(ani->sequences, free_sequence_callback, NULL);
		hashtable_free(ani->sequences);
		free(ani);
		free(basename);
		return NULL;
	}

	AnimationLoadData *data = malloc(sizeof(AnimationLoadData));
	data->ani = ani;
	data->basename = basename;
	return data;
}

static void *check_sequence_frame_callback(void *key, void *value, void *arg) {
	intptr_t sprite_count = (intptr_t)arg;
	AniSequence *seq = value;
	int errors = 0;
	for(int i = 0; i < seq->length; i++) {
		if(seq->frames[i].spriteidx >= sprite_count) {
			log_warn("Animation sequence %s: Sprite index %d is higher than sprite_count.",(char *)key,seq->frames[i].spriteidx);
			errors++;
		}
	}

	return (void *)(intptr_t)errors;
}

void* load_animation_end(void *opaque, const char *filename, uint flags) {
	AnimationLoadData *data = opaque;

	if(opaque == NULL) {
		return NULL;
	}

	Animation *ani = data->ani;
	ani->sprites = calloc(ani->sprite_count, sizeof(Sprite*));

	char buf[strlen(data->basename) + sizeof(".frame0000")];

	for(int i = 0; i < ani->sprite_count; ++i) {
		snprintf(buf, sizeof(buf), "%s.frame%04d", data->basename, i);
		Resource *res = get_resource(RES_SPRITE, buf, flags);

		if(res == NULL) {
			log_warn("Animation frame '%s' not found but @sprite_count was %d",buf,ani->sprite_count);
			unload_animation(ani);
			ani = NULL;
			break;
		}

		ani->sprites[i] = res->data;
	}

	if(hashtable_foreach(ani->sequences, check_sequence_frame_callback, (void *)(intptr_t)ani->sprite_count) != 0) {
		unload_animation(ani);
		ani = NULL;
	}

	free(data->basename);
	free(data);

	return ani;
}

void unload_animation(void *vani) {
	Animation *ani = vani;
	hashtable_foreach(ani->sequences, free_sequence_callback, NULL);
	hashtable_free(ani->sequences);
	free(ani->sprites);
	free(ani);
}

Animation *get_ani(const char *name) {
	return get_resource(RES_ANIM, name, RESF_DEFAULT)->data;
}

AniSequence *get_ani_sequence(Animation *ani, const char *seqname) {
	AniSequence *seq = hashtable_get_string(ani->sequences,seqname);
	if(seq == NULL) {
		log_fatal("Sequence '%s' not found.",seqname);
	}

	return seq;
}

Sprite* animation_get_frame(Animation *ani, AniSequence *seq, int seqframe) {
	AniSequenceFrame *f = &seq->frames[seqframe%seq->length];
	if(f->mirrored) {
		memcpy(&ani->transformed_sprite,ani->sprites[f->spriteidx],sizeof(Sprite));
		// XXX: maybe add sprite_flip() or something
		FloatRect *tex_area = &ani->transformed_sprite.tex_area;
		tex_area->x += tex_area->w;
		tex_area->w *= -1;
		return &ani->transformed_sprite;
	}

	assert(f->spriteidx < ani->sprite_count);
	return ani->sprites[f->spriteidx];
}

