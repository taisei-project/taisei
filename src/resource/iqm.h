/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

/*
 * Adapted from https://github.com/lsalzman/iqm/blob/master/iqm.h
 */

#define IQM_MAGIC "INTERQUAKEMODEL"
#define IQM_VERSION 2

typedef enum IQMVertexArrayType {
	IQM_POSITION     = 0,
	IQM_TEXCOORD     = 1,
	IQM_NORMAL       = 2,
	IQM_TANGENT      = 3,
	IQM_BLENDINDEXES = 4,
	IQM_BLENDWEIGHTS = 5,
	IQM_COLOR        = 6,
	IQM_CUSTOM       = 0x10
} IQMVertexArrayType;

typedef enum IQMVertexFormat {
	IQM_BYTE   = 0,
	IQM_UBYTE  = 1,
	IQM_SHORT  = 2,
	IQM_USHORT = 3,
	IQM_INT    = 4,
	IQM_UINT   = 5,
	IQM_HALF   = 6,
	IQM_FLOAT  = 7,
	IQM_DOUBLE = 8,
} IQMVertexFormat;

typedef enum IQMAnimFlags {
	IQM_LOOP = 1 << 0
} IQMAnimFlags;

#define IQM_FIELDS(...) \
	union { \
		struct __VA_ARGS__; \
		uint32_t u32_array[sizeof(struct __VA_ARGS__) / sizeof(uint32_t)]; \
	}

typedef struct IQMHeader {
	uint8_t magic[sizeof(IQM_MAGIC)];

	IQM_FIELDS({
		uint32_t version;
		uint32_t filesize;
		uint32_t flags;
		uint32_t num_text, ofs_text;
		uint32_t num_meshes, ofs_meshes;
		uint32_t num_vertexarrays, num_vertexes, ofs_vertexarrays;
		uint32_t num_triangles, ofs_triangles, ofs_adjacency;
		uint32_t num_joints, ofs_joints;
		uint32_t num_poses, ofs_poses;
		uint32_t num_anims, ofs_anims;
		uint32_t num_frames, num_framechannels, ofs_frames, ofs_bounds;
		uint32_t num_comment, ofs_comment;
		uint32_t num_extensions, ofs_extensions;
	});
} IQMHeader;

typedef struct IQMMesh {
	IQM_FIELDS({
		uint32_t name;
		uint32_t material;
		uint32_t first_vertex, num_vertexes;
		uint32_t first_triangle, num_triangles;
	});
} IQMMesh;

typedef struct IQMTriangle {
	IQM_FIELDS({
		uint32_t vertex[3];
	});
} IQMTriangle;

typedef struct IQMAdjacency {
	IQM_FIELDS({
		uint32_t triangle[3];
	});
} IQMAdjacency;

typedef struct IQMJoint {
	IQM_FIELDS({
		uint32_t name;
		int32_t parent;
		float translate[3], rotate[4], scale[3];
	});
} IQMJoint;

typedef struct IQMPose {
	IQM_FIELDS({
		int32_t parent;
		uint32_t mask;
		float channeloffset[10];
		float channelscale[10];
	});
} IQMPose;

typedef struct IQMAnim {
	IQM_FIELDS({
		uint32_t name;
		uint32_t first_frame, num_frames;
		float framerate;
		uint32_t flags;  /// IQMAnimFlags
	});
} IQMAnim;

typedef struct IQMVertexArray {
	IQM_FIELDS({
		uint32_t type;  /// IQMVertexArrayType
		uint32_t flags;
		uint32_t format;   /// IQMVertexFormat
		uint32_t size;
		uint32_t offset;
	});
} IQMVertexArray;

typedef struct IQMBounds {
	IQM_FIELDS({
		float bbmin[3], bbmax[3];
		float xyradius, radius;
	});
} IQMBounds;

#undef IQM_FIELDS
