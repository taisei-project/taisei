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

// #define SPAM(...) log_debug(__VA_ARGS__)
#define SPAM(...) ((void)0)

bool splayer_init(StreamPlayer *plr, int num_channels, const AudioStreamSpec *dst_spec) {
	memset(plr, 0, sizeof(*plr));
	plr->num_channels = num_channels,
	plr->channels = calloc(sizeof(*plr->channels), num_channels);
	plr->dst_spec = *dst_spec;

	for(int i = 0; i < num_channels; ++i) {
		StreamPlayerChannel *chan = plr->channels + i;
		chan->fade_target = 1;
		chan->gain = 1;
		chan->paused = true;
		alist_append(&plr->channel_history, chan);
	}

	log_debug("Player spec: %iHz; %i chans; format=%i",
		plr->dst_spec.sample_rate,
		plr->dst_spec.channels,
		plr->dst_spec.sample_format
	);

	return true;
}

static inline void splayer_history_move_to_front(StreamPlayer *plr, StreamPlayerChannel *pchan) {
	alist_unlink(&plr->channel_history, pchan);
	alist_push(&plr->channel_history, pchan);
}

static inline void splayer_history_move_to_back(StreamPlayer *plr, StreamPlayerChannel *pchan) {
	alist_unlink(&plr->channel_history, pchan);
	alist_append(&plr->channel_history, pchan);
}

static void free_channel(StreamPlayerChannel *chan) {
	SDL_FreeAudioStream(chan->pipe);
}

void splayer_shutdown(StreamPlayer *plr) {
	for(int i = 0; i < plr->num_channels; ++i) {
		free_channel(plr->channels + i);
	}

	free(plr->channels);
}

static inline void splayer_stream_ended(StreamPlayer *plr, int chan) {
	SPAM("Audio stream ended on channel %i", chan);
	splayer_halt(plr, chan);
}

static size_t splayer_process_channel(StreamPlayer *plr, int chan, size_t bufsize, void *buffer) {
	AudioStreamReadFlags rflags = 0;
	StreamPlayerChannel *pchan = plr->channels + chan;

	if(pchan->paused || !pchan->stream) {
		return 0;
	}

	if(pchan->looping) {
		rflags |= ASTREAM_READ_LOOP;
	}

	uint8_t *buf = buffer;
	uint8_t *buf_end = buf + bufsize;
	AudioStream *astream = pchan->stream;
	SDL_AudioStream *pipe = pchan->pipe;

	if(pipe) {
		// convert/resample

		do {
			uint8_t staging_buffer[bufsize];
			ssize_t read = SDL_AudioStreamGet(pipe, buf, buf_end - buf);

			if(UNLIKELY(read < 0)) {
				log_sdl_error(LOG_ERROR, "SDL_AudioStreamGet");
				break;
			}

			buf += read;

			if(buf >= buf_end) {
				break;
			}

			read = astream_read_into_sdl_stream(astream, pipe, sizeof(staging_buffer), staging_buffer, rflags);

			if(read <= 0) {
				SDL_AudioStreamFlush(pipe);

				if(SDL_AudioStreamAvailable(pipe) <= 0)  {
					splayer_stream_ended(plr, chan);
					break;
				}
			}
		} while(buf < buf_end);
	} else {
		// direct stream

		ssize_t read = astream_read(astream, bufsize, buf, rflags | ASTREAM_READ_MAX_FILL);

		if(read > 0) {
			buf += read;
		} else if(read == 0) {
			splayer_stream_ended(plr, chan);
		}
	}

	assert(buf <= buf_end);
	return bufsize - (buf_end - buf);
}

INLINE int chan_integer_gain(StreamPlayerChannel *pchan, float gain) {
	return pchan->gain * pchan->fade_gain * gain * SDL_MIX_MAXVOLUME;
}

void splayer_process(StreamPlayer *plr, size_t bufsize, void *buffer) {
	/*
	 * NOTE/FIXME: The SDL wiki has the following to say about SDL_MixAudioFormat:
	 *
	 * > Do not use this function for mixing together more than two streams of sample data. The output from repeated
	 * > application of this function may be distorted by clipping, because there is no accumulator with greater range
	 * > than the input (not to mention this being an inefficient way of doing it). Use mixing functions from SDL_mixer,
	 * > OpenAL, or write your own mixer instead.
	 *
	 * https://wiki.libsdl.org/SDL_MixAudioFormat
	 *
	 * However, SDL_mixer itself uses this function to mix multiple streams, so this appears to be at least somewhat
	 * inaccurate. That said, it's probably worthwhile to write a more efficient/specialized mixing function that does
	 * not clip samples.
	 */

	if(plr->paused) {
		return;
	}

	float gain = plr->gain;
	size_t frame_size = plr->dst_spec.frame_size;
	SDL_AudioFormat sample_format = plr->dst_spec.sample_format;
	int num_channels = plr->num_channels;

	for(int i = 0; i < num_channels; ++i) {
		uint8_t staging_buffer[bufsize];
		size_t chan_bytes = splayer_process_channel(plr, i, sizeof(staging_buffer), staging_buffer);

		if(chan_bytes) {
			assert(chan_bytes <= bufsize);
			assert(chan_bytes % frame_size == 0);

			StreamPlayerChannel *pchan = plr->channels + i;

			if(pchan->fade_step) {
				uint8_t *psrc = staging_buffer;
				uint8_t *pdst = buffer;
				uint8_t *pdst_end = pdst + chan_bytes;

				do {
					int igain = chan_integer_gain(pchan, gain);
					SDL_MixAudioFormat(pdst, psrc, sample_format, frame_size, igain);
					psrc += frame_size;
					pdst += frame_size;

					if(pchan->fade_step && fapproach_p(&pchan->fade_gain, pchan->fade_target, pchan->fade_step) == pchan->fade_target) {
						pchan->fade_step = 0;

						if(pchan->fade_target == 0) {
							splayer_stream_ended(plr, i);
						}
					}
				} while(pdst < pdst_end);
			} else {
				int igain = chan_integer_gain(pchan, gain);
				SDL_MixAudioFormat(buffer, staging_buffer, plr->dst_spec.sample_format, chan_bytes, igain);
			}
		}
	}
}

static inline float calc_fade_step(StreamPlayer *plr, float fadetime) {
	return 1 / (fadetime * plr->dst_spec.sample_rate);
}

static bool splayer_validate_channel(StreamPlayer *plr, int chan) {
	if(chan < 0 || chan >= plr->num_channels) {
		log_error("Bad channel %i", chan);
		return false;
	}

	return true;
}

bool splayer_play(StreamPlayer *plr, int chan, AudioStream *stream, bool loop, double position, double fadein) {
	if(!splayer_validate_channel(plr, chan)) {
		return false;
	}

	if(UNLIKELY(astream_seek(stream, astream_util_time_to_offset(stream, position)) < 0)) {
		log_error("astream_seek() failed");
		goto fail;
	}

	if(UNLIKELY(fadein < 0)) {
		log_error("Bad fadein time value %f", fadein);
		goto fail;
	}

	StreamPlayerChannel *pchan = plr->channels + chan;

	pchan->stream = stream;
	pchan->looping = loop;
	pchan->paused = false;
	pchan->fade_target = 1;

	splayer_history_move_to_back(plr, pchan);

	if(fadein == 0) {
		pchan->fade_gain = 1;
		pchan->fade_step = 0;
	} else {
		pchan->fade_gain = 0;
		pchan->fade_step = calc_fade_step(plr, fadein);
	}

	SPAM("Stream spec: %iHz; %i chans; format=%i",
		stream->spec.sample_rate,
		stream->spec.channels,
		stream->spec.sample_format
	);

	SPAM("Player spec: %iHz; %i chans; format=%i",
		plr->dst_spec.sample_rate,
		plr->dst_spec.channels,
		plr->dst_spec.sample_format
	);

	if(LIKELY(astream_spec_equals(&stream->spec, &plr->dst_spec))) {
		SPAM("Stream needs no conversion");

		if(pchan->pipe) {
			SDL_FreeAudioStream(pchan->pipe);
			pchan->pipe = NULL;
			SPAM("Destroyed conversion pipe");
		}
	} else {
		SPAM("Stream needs a conversion");

		if(pchan->pipe && UNLIKELY(!astream_spec_equals(&stream->spec, &pchan->src_spec))) {
			SDL_FreeAudioStream(pchan->pipe);
			pchan->pipe = NULL;
			SPAM("Existing conversion pipe can't be reused; destroyed");
		}

		if(pchan->pipe) {
			SDL_AudioStreamClear(pchan->pipe);
			SPAM("Reused an existing conversion pipe");
		} else {
			pchan->pipe = astream_create_sdl_stream(stream, &plr->dst_spec);
			pchan->src_spec = stream->spec;
			SPAM("Created a new conversion pipe");
		}
	}

	return true;

fail:
	splayer_halt(plr, chan);
	return false;
}

bool splayer_pause(StreamPlayer *plr, int chan) {
	if(!splayer_validate_channel(plr, chan)) {
		return false;
	}

	if(UNLIKELY(plr->channels[chan].paused)) {
		log_warn("Channel %i is already paused", chan);
		return false;
	}

	plr->channels[chan].paused = true;
	return true;
}

bool splayer_resume(StreamPlayer *plr, int chan) {
	if(!splayer_validate_channel(plr, chan)) {
		return false;
	}

	StreamPlayerChannel *pchan = plr->channels + chan;

	if(UNLIKELY(!pchan->paused)) {
		log_warn("Channel %i is not paused", chan);
		return false;
	}

	if(UNLIKELY(!pchan->stream)) {
		log_error("Channel %i has no stream", chan);
		return false;
	}

	pchan->paused = false;
	return true;
}

void splayer_halt(StreamPlayer *plr, int chan) {
	if(!splayer_validate_channel(plr, chan)) {
		return;
	}

	StreamPlayerChannel *pchan = plr->channels + chan;
	pchan->stream = NULL;
	pchan->paused = true;
	pchan->looping = false;
	pchan->fade_gain = 0;
	pchan->fade_step = 0;
	pchan->fade_target = 1;
	pchan->gain = 1;

	splayer_history_move_to_front(plr, pchan);
}

bool splayer_fadeout(StreamPlayer *plr, int chan, double fadeout) {
	if(!splayer_validate_channel(plr, chan)) {
		return false;
	}

	if(UNLIKELY(fadeout <= 0)) {
		log_error("Bad fadeout time value %f", fadeout);
		return false;
	}

	StreamPlayerChannel *pchan = plr->channels + chan;
	pchan->fade_target = 0;
	pchan->fade_step = calc_fade_step(plr, fadeout);

	return true;
}

double splayer_seek(StreamPlayer *plr, int chan, double position) {
	if(!splayer_validate_channel(plr, chan)) {
		return false;
	}

	StreamPlayerChannel *pchan = plr->channels + chan;

	if(UNLIKELY(!pchan->stream)) {
		log_error("Channel %i has no stream", chan);
		return -1;
	}

	ssize_t ofs = astream_util_time_to_offset(pchan->stream, position);
	ofs = astream_seek(pchan->stream, ofs);

	if(UNLIKELY(ofs < 0)) {
		log_error("astream_seek() failed");
		return -1;
	}

	position = astream_util_offset_to_time(pchan->stream, ofs);
	return position;
}

double splayer_tell(StreamPlayer *plr, int chan) {
	if(!splayer_validate_channel(plr, chan)) {
		return false;
	}

	StreamPlayerChannel *pchan = plr->channels + chan;

	if(UNLIKELY(!pchan->stream)) {
		log_error("Channel %i has no stream", chan);
		return -1;
	}

	ssize_t ofs = astream_tell(pchan->stream);

	if(UNLIKELY(ofs < 0)) {
		log_error("astream_tell() failed");
		return -1;
	}

	double position = astream_util_offset_to_time(pchan->stream, ofs);
	return position;
}

bool splayer_global_pause(StreamPlayer *plr) {
	if(UNLIKELY(plr->paused)) {
		log_warn("Player is already paused");
		return false;
	}

	plr->paused = true;
	return true;
}

bool splayer_global_resume(StreamPlayer *plr) {
	if(UNLIKELY(!plr->paused)) {
		log_warn("Player is not paused");
		return false;
	}

	plr->paused = false;
	return true;
}

bool splayer_is_looping(StreamPlayer *plr, int chan) {
	if(!splayer_validate_channel(plr, chan)) {
		return false;
	}

	StreamPlayerChannel *pchan = plr->channels + chan;
	bool loop = pchan->stream && pchan->looping;

	return loop;
}

static bool is_fading_out(StreamPlayerChannel *pchan) {
	return pchan->fade_step && pchan->fade_target == 0;
}

BGMStatus splayer_util_bgmstatus(StreamPlayer *plr, int chan) {
	if(!splayer_validate_channel(plr, chan)) {
		return false;
	}

	BGMStatus status = BGM_PLAYING;
	StreamPlayerChannel *pchan = plr->channels + chan;

	if(!pchan->stream || is_fading_out(pchan)) {
		status = BGM_STOPPED;
	} else if(plr->paused) {
		status = BGM_PAUSED;
	}

	return status;
}

int splayer_pick_channel(StreamPlayer *plr) {
	StreamPlayerChannel *pick = plr->channel_history.first;

	for(StreamPlayerChannel *pchan = plr->channel_history.first; pchan; pchan = pchan->next) {
		if(pchan->stream == NULL) {
			// We have a free channel, perfect.
			// If a free channel exists, then it is always at the head of the history list (if history works correctly).
			assume(pchan == plr->channel_history.first);
			pick = pchan;
			break;
		}

		// No free channel, so we try to pick the oldest one that isn't looping.
		// We could do better, e.g. pick the one with least time remaining, but this should be ok for short sounds.

		if(!pchan->looping) {
			pick = pchan;
			break;
		}
	}

	int id = pick - plr->channels;

	return id;
}
