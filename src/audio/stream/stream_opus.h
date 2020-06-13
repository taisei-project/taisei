/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_audio_stream_stream_opus_h
#define IGUARD_audio_stream_stream_opus_h

#include "taisei.h"

#include "stream.h"

#include <opusfile.h>

bool astream_opus_open(AudioStream *stream, SDL_RWops *rw);

#endif // IGUARD_audio_stream_stream_opus_h
