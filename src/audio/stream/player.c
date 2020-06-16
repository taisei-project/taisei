/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "player.h"
#include "util.h"

bool splayer_init(StreamPlayer *plr, const AudioStreamSpec *dst_spec) {
	if(UNLIKELY(dst_spec->sample_format != AUDIO_F32SYS)) {
		log_error("Unsupported sample format %i", dst_spec->sample_format);
		return false;
	}

	memset(plr, 0, sizeof(*plr));
	plr->dst_spec = *dst_spec;
	plr->paused = true;
	plr->mutex = SDL_CreateMutex();

	log_debug("Player spec: %iHz; %i chans; format=%i",
		plr->dst_spec.sample_rate,
		plr->dst_spec.channels,
		plr->dst_spec.sample_format
	);

	return true;
}

void splayer_lock(StreamPlayer *plr) {
	SDL_LockMutex(plr->mutex);
}

void splayer_unlock(StreamPlayer *plr) {
	SDL_UnlockMutex(plr->mutex);
}

void splayer_shutdown(StreamPlayer *plr) {
	SDL_FreeAudioStream(plr->pipe);
	SDL_DestroyMutex(plr->mutex);
}

static inline void splayer_stream_ended(StreamPlayer *plr) {
	log_debug("Audio stream ended");
	splayer_halt(plr);
}

size_t splayer_process(StreamPlayer *plr, size_t bufsize, void *buffer, StreamPlayerProcessFlags flags) {
	splayer_lock(plr);

	bool silence_pad = flags & SPLAYER_SILENCE_PAD;
	float gain = plr->gain;

	union {
		uint8_t *bytes;
		float *samples;
	} stream = { buffer };
	uint8_t *stream_end = stream.bytes + bufsize;
	size_t sample_size = sizeof(*stream.samples);
	size_t num_samples = bufsize / sample_size;
	size_t num_samples_filled = 0;

	if(plr->paused) {
		if(silence_pad) {
			memset(stream.bytes, 0, bufsize);
		}

		splayer_unlock(plr);
		return 0;
	}

	AudioStreamReadFlags rflags = 0;

	if(plr->looping) {
		rflags |= ASTREAM_READ_LOOP;
	}

	if(plr->stream) {
		AudioStream *astream = plr->stream;
		SDL_AudioStream *pipe = plr->pipe;

		if(pipe) {
			// convert/resample

			do {
				uint8_t staging_buffer[bufsize];
				ssize_t read = SDL_AudioStreamGet(pipe, stream.bytes, stream_end - stream.bytes);

				if(read < 0) {
					log_sdl_error(LOG_ERROR, "SDL_AudioStreamGet");
					break;
				}

				stream.bytes += read;

				if(stream.bytes >= stream_end) {
					break;
				}

				read = astream_read_into_sdl_stream(astream, pipe, sizeof(staging_buffer), staging_buffer, rflags);

				if(read == 0) {
					SDL_AudioStreamFlush(pipe);

					if(SDL_AudioStreamAvailable(pipe) <= 0)  {
						splayer_stream_ended(plr);
						break;
					}
				} else if(read < 0) {
					break;
				}
			} while(stream.bytes < stream_end);

			num_samples_filled = (bufsize - (stream_end - stream.bytes)) / sample_size;
		} else {
			// direct stream

			ssize_t read = astream_read(astream, bufsize, stream.bytes, rflags | ASTREAM_READ_MAX_FILL);

			if(read > 0) {
				num_samples_filled = read / sample_size;
			} else if(read == 0) {
				splayer_stream_ended(plr);
			}
		}
	}

	stream.samples = buffer;

	if(plr->fade_step) {
		int chans = plr->dst_spec.channels;

		for(int i = 0; i < num_samples_filled; i += chans) {
			for(int chan = 0; chan < chans; ++chan) {
				stream.samples[i + chan] *= gain * plr->fade_gain;
			}

			if(plr->fade_step && fapproach_p(&plr->fade_gain, plr->fade_target, plr->fade_step) == plr->fade_target) {
				plr->fade_step = 0;

				if(plr->fade_target == 0) {
					splayer_stream_ended(plr);
				}
			}
		}
	} else {
		gain *= plr->fade_gain;
		for(int i = 0; i < num_samples_filled; ++i) {
			stream.samples[i] *= gain;
		}
	}

	if(silence_pad) {
		memset(stream.samples + num_samples_filled, 0, (num_samples - num_samples_filled) * sample_size);
	}

	splayer_unlock(plr);
	return num_samples_filled * sample_size;
}

static inline float calc_fade_step(StreamPlayer *plr, float fadetime) {
	return 1 / (fadetime * plr->dst_spec.sample_rate);
}

bool splayer_play(StreamPlayer *plr, AudioStream *stream, bool loop, double position, double fadein) {
	splayer_lock(plr);

	if(UNLIKELY(astream_seek(stream, astream_util_time_to_offset(stream, position)) < 0)) {
		log_error("astream_seek() failed");
		goto fail;
	}

	if(UNLIKELY(fadein < 0)) {
		log_error("Bad fadein time value %f", fadein);
		goto fail;
	}

	plr->stream = stream;
	plr->looping = loop;
	plr->paused = false;
	plr->fade_target = 1;

	if(fadein == 0) {
		plr->fade_gain = 1;
		plr->fade_step = 0;
	} else {
		plr->fade_gain = 0;
		plr->fade_step = calc_fade_step(plr, fadein);
	}

	log_debug("Stream spec: %iHz; %i chans; format=%i",
		stream->spec.sample_rate,
		stream->spec.channels,
		stream->spec.sample_format
	);

	log_debug("Player spec: %iHz; %i chans; format=%i",
		plr->dst_spec.sample_rate,
		plr->dst_spec.channels,
		plr->dst_spec.sample_format
	);

	if(LIKELY(astream_spec_equals(&stream->spec, &plr->dst_spec))) {
		log_debug("Stream needs no conversion");

		if(plr->pipe) {
			SDL_FreeAudioStream(plr->pipe);
			plr->pipe = NULL;
			log_debug("Destroyed conversion pipe");
		}
	} else {
		log_debug("Stream needs a conversion");

		if(plr->pipe && UNLIKELY(!astream_spec_equals(&stream->spec, &plr->src_spec))) {
			SDL_FreeAudioStream(plr->pipe);
			plr->pipe = NULL;
			log_debug("Existing conversion pipe can't be reused; destroyed");
		}

		if(plr->pipe) {
			SDL_AudioStreamClear(plr->pipe);
			log_debug("Reused an existing conversion pipe");
		} else {
			plr->pipe = astream_create_sdl_stream(stream, plr->dst_spec);
			plr->src_spec = stream->spec;
			log_debug("Created a new conversion pipe");
		}
	}

	splayer_unlock(plr);

	return true;

fail:
	splayer_halt(plr);
	splayer_unlock(plr);
	return false;
}

bool splayer_pause(StreamPlayer *plr) {
	splayer_lock(plr);

	if(UNLIKELY(plr->paused)) {
		log_warn("Player is already paused");
		splayer_unlock(plr);
		return false;
	}

	plr->paused = true;
	splayer_unlock(plr);
	return true;
}

bool splayer_resume(StreamPlayer *plr) {
	splayer_lock(plr);

	if(UNLIKELY(!plr->paused)) {
		log_warn("Player is not paused");
		goto fail;
	}

	if(UNLIKELY(!plr->stream)) {
		log_error("Player has no stream");
		goto fail;
	}

	plr->paused = false;
	splayer_unlock(plr);
	return true;

fail:
	splayer_unlock(plr);
	return false;
}

void splayer_halt(StreamPlayer *plr) {
	splayer_lock(plr);
	plr->stream = NULL;
	plr->paused = true;
	plr->looping = false;
	plr->fade_gain = 0;
	plr->fade_step = 0;
	plr->fade_target = 1;
	splayer_unlock(plr);
}

bool splayer_fadeout(StreamPlayer *plr, double fadeout) {
	if(UNLIKELY(fadeout <= 0)) {
		log_error("Bad fadeout time value %f", fadeout);
		return false;
	}

	splayer_lock(plr);
	plr->fade_target = 0;
	plr->fade_step = calc_fade_step(plr, fadeout);
	splayer_unlock(plr);

	return true;
}

double splayer_seek(StreamPlayer *plr, double position) {
	splayer_lock(plr);

	if(UNLIKELY(!plr->stream)) {
		log_error("Player has no stream");
		goto fail;
	}

	ssize_t ofs = astream_util_time_to_offset(plr->stream, position);
	ofs = astream_seek(plr->stream, ofs);

	if(UNLIKELY(ofs < 0)) {
		log_error("astream_seek() failed");
		goto fail;
	}

	position = astream_util_offset_to_time(plr->stream, ofs);
	splayer_unlock(plr);

	return position;

fail:
	splayer_unlock(plr);
	return -1;
}

double splayer_tell(StreamPlayer *plr) {
	splayer_lock(plr);

	if(UNLIKELY(!plr->stream)) {
		log_error("Player has no stream");
		goto fail;
	}

	ssize_t ofs = astream_tell(plr->stream);

	if(UNLIKELY(ofs < 0)) {
		log_error("astream_tell() failed");
		goto fail;
	}

	double position = astream_util_offset_to_time(plr->stream, ofs);
	splayer_unlock(plr);

	return position;

fail:
	splayer_unlock(plr);
	return -1;
}

static bool is_fading_out(StreamPlayer *plr) {
	return plr->fade_step && plr->fade_target == 0;
}

BGMStatus splayer_util_bgmstatus(StreamPlayer *plr) {
	BGMStatus status = BGM_PLAYING;
	splayer_lock(plr);

	if(!plr->stream || is_fading_out(plr)) {
		status = BGM_STOPPED;
	} else if(plr->paused) {
		status = BGM_PAUSED;
	}

	splayer_unlock(plr);
	return status;
}
