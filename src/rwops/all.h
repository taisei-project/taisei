/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_rwops_all_h
#define IGUARD_rwops_all_h

#include "taisei.h"

#include "rwops_autobuf.h"
#include "rwops_crc32.h"
#include "rwops_ro.h"
#include "rwops_segment.h"
#include "rwops_sha256.h"
#include "rwops_zlib.h"
#include "rwops_zstd.h"

#ifdef TAISEI_BUILDCONF_USE_ZIP
#include "rwops_zipfile.h"
#endif

#ifdef DEBUG
#include "rwops_trace.h"
#endif

#endif // IGUARD_rwops_all_h
