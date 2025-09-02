/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "player.h"
#include "util.h"

// #define SPAM(...) log_debug(__VA_ARGS__)
#define SPAM(...) ((void)0)

typedef float sample_t;

struct stereo_frame {
	sample_t l, r;
};

union audio_buffer {
	uint8_t *bytes;
	sample_t *samples;
	struct stereo_frame *frames;
};

bool splayer_init(StreamPlayer *plr, int num_channels, const AudioStreamSpec *dst_spec) {
	*plr = (typeof(*plr)) {};

	if(dst_spec->sample_format != SDL_AUDIO_F32) {
		log_error("Unsupported audio format: 0x%4x", dst_spec->sample_format);
		return false;
	}

	if(dst_spec->channels != 2) {
		log_error("Unsupported number of audio channels: %i", dst_spec->channels);
		return false;
	}

	assert(dst_spec->frame_size == sizeof(struct stereo_frame));

	plr->num_channels = num_channels,
	plr->channels = ALLOC_ARRAY(num_channels, typeof(*plr->channels));
	plr->dst_spec = *dst_spec;

	for(int i = 0; i < num_channels; ++i) {
		StreamPlayerChannel *chan = plr->channels + i;
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
	SDL_DestroyAudioStream(chan->pipe);
}

void splayer_shutdown(StreamPlayer *plr) {
	for(int i = 0; i < plr->num_channels; ++i) {
		free_channel(plr->channels + i);
	}

	mem_free(plr->channels);
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
			ssize_t read = SDL_GetAudioStreamData(pipe, buf, buf_end - buf);

			if(UNLIKELY(read < 0)) {
				log_sdl_error(LOG_ERROR, "SDL_GetAudioStreamData");
				break;
			}

			buf += read;

			if(buf >= buf_end) {
				break;
			}

			read = astream_read_into_sdl_stream(astream, pipe, sizeof(staging_buffer), staging_buffer, rflags);

			if(read <= 0) {
				SDL_FlushAudioStream(pipe);

				if(SDL_GetAudioStreamAvailable(pipe) <= 0)  {
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

void splayer_process(StreamPlayer *plr, size_t bufsize, void *vbuffer) {
	if(plr->paused) {
		return;
	}

	float gain = plr->gain;
	int num_channels = plr->num_channels;
	union audio_buffer out_buffer = { vbuffer };

	for(int i = 0; i < num_channels; ++i) {
		uint8_t staging_buffer_bytes[bufsize];
		union audio_buffer staging_buffer = { staging_buffer_bytes };
		size_t chan_bytes = splayer_process_channel(plr, i, sizeof(staging_buffer_bytes), staging_buffer_bytes);

		if(chan_bytes) {
			assert(chan_bytes <= bufsize);
			assert(chan_bytes % sizeof(struct stereo_frame) == 0);

			StreamPlayerChannel *pchan = plr->channels + i;
			float chan_gain = gain * pchan->gain;
			uint num_staging_frames = chan_bytes / sizeof(struct stereo_frame);
			uint fade_steps = pchan->fade.num_steps;

			if(fade_steps) {
				float fade_step = pchan->fade.step;
				float fade_gain = pchan->fade.gain;

				if(fade_steps > num_staging_frames) {
					fade_steps = num_staging_frames;
				}

				for(uint i = 0; i < fade_steps; ++i) {
					float g = fade_gain + fade_step * i;
					staging_buffer.frames[i].l *= g;
					staging_buffer.frames[i].r *= g;
				}

				if((pchan->fade.num_steps -= fade_steps) == 0) {
					// fade finished

					if(pchan->fade.target == 0) {
						splayer_stream_ended(plr, i);
						continue;
					}

					pchan->fade.gain = pchan->fade.target;
					chan_gain *= pchan->fade.gain;
				} else {
					pchan->fade.gain += fade_step * fade_steps;
				}
			} else {
				chan_gain *= pchan->fade.gain;
			}

			for(uint i = 0; i < num_staging_frames; ++i) {
				out_buffer.frames[i].l += staging_buffer.frames[i].l * chan_gain;
				out_buffer.frames[i].r += staging_buffer.frames[i].r * chan_gain;
			}
		}
	}
}

static bool splayer_validate_channel(StreamPlayer *plr, int chan) {
	if(chan < 0 || chan >= plr->num_channels) {
		log_error("Bad channel %i", chan);
		return false;
	}

	return true;
}

static void splayer_set_fade(StreamPlayer *plr, StreamPlayerChannel *pchan, float from, float to, float time) {
	if(time == 0) {
		pchan->fade.target = to;
		pchan->fade.gain = to;
		pchan->fade.num_steps = 0;
		pchan->fade.step = 0;
	} else {
		pchan->fade.target = to;
		pchan->fade.gain = from;
		pchan->fade.num_steps = lroundf(plr->dst_spec.sample_rate * time);
		pchan->fade.step = (to - from) / pchan->fade.num_steps;
	}
}

bool splayer_play(StreamPlayer *plr, int chan, AudioStream *stream, bool loop, float gain, double position, double fadein) {
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
	pchan->gain = gain;

	splayer_history_move_to_back(plr, pchan);
	splayer_set_fade(plr, pchan, 0, 1, fadein);

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
			SDL_DestroyAudioStream(pchan->pipe);
			pchan->pipe = NULL;
			SPAM("Destroyed conversion pipe");
		}
	} else {
		SPAM("Stream needs a conversion");

		if(pchan->pipe && UNLIKELY(!astream_spec_equals(&stream->spec, &pchan->src_spec))) {
			SDL_DestroyAudioStream(pchan->pipe);
			pchan->pipe = NULL;
			SPAM("Existing conversion pipe can't be reused; destroyed");
		}

		if(pchan->pipe) {
			SDL_ClearAudioStream(pchan->pipe);
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
	pchan->gain = 0;
	pchan->fade.gain = 0;
	pchan->fade.step = 0;
	pchan->fade.num_steps = 0;
	pchan->fade.target = 0;
	splayer_history_move_to_front(plr, pchan);
}

static bool splayer_fade(StreamPlayer *plr, int chan, double fadetime, double fadetarget) {
	if(!splayer_validate_channel(plr, chan)) {
		return false;
	}

	if(UNLIKELY(fadetime < 0)) {
		log_error("Bad fade time value %f", fadetime);
		return false;
	}

	StreamPlayerChannel *pchan = plr->channels + chan;
	splayer_set_fade(plr, pchan, pchan->fade.gain, fadetarget, fadetime);

	return true;

}

bool splayer_fadeout(StreamPlayer *plr, int chan, double fadetime) {
	return splayer_fade(plr, chan, fadetime, 0);
}

bool splayer_fadein(StreamPlayer *plr, int chan, double fadetime) {
	return splayer_fade(plr, chan, fadetime, 1);
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
	return pchan->fade.num_steps && pchan->fade.target == 0;
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
