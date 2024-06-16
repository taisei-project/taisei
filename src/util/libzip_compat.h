/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <zip.h>

#ifndef TAISEI_BUILDCONF_HAVE_ZIP_COMPRESSION_METHOD_SUPPORTED

#define zip_compression_method_supported _taisei_zip_compression_method_supported
attr_unused static int zip_compression_method_supported(zip_int32_t method, int compress) {
	// assume bare minimum
	return (method == ZIP_CM_DEFLATE || method == ZIP_CM_STORE);
}

#endif

#ifndef ZIP_CM_ZSTD
#define ZIP_CM_ZSTD 93
#endif
