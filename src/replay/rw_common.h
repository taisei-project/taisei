/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_replay_rw_common_h
#define IGUARD_replay_rw_common_h

#include "taisei.h"

#include "struct.h"

uint32_t replay_struct_stage_metadata_checksum(ReplayStage *stg, uint16_t version);

extern uint8_t replay_magic_header[REPLAY_MAGIC_HEADER_SIZE];

#endif // IGUARD_replay_rw_common_h
