/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "build_config.h"
#include "rwops_zlib.h"
#include "rwops_segment.h"
#include "rwops_autobuf.h"
#include "rwops_pipe.h"

#ifndef DISABLE_ZIP
#include "rwops_zipfile.h"
#endif
