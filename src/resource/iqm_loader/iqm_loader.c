/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "iqm_loader.h"
#include "iqm.h"

#include "log.h"
#include "util.h"

// arbitrary allocation limits (raise as needed)
#define MAX_MESHES (1 << 8)
#define MAX_VERTICES (1 << 24)
#define MAX_TRIANGLES (1 << 22)
#define MAX_VERTEX_ARRAYS (1 << 8)

#define NUM_REQUIRED_VERTEX_ARRAYS 4

typedef union VertexArrayIndices {
	struct { int position, texcoord, normal, tangent; };
	int indices[NUM_REQUIRED_VERTEX_ARRAYS];
} VertexArrayIndices;

static const char *iqm_va_type_str(uint32_t type) {
	switch(type) {
		case IQM_POSITION:      return "position";
		case IQM_NORMAL:        return "normal";
		case IQM_TANGENT:       return "tangent";
		case IQM_TEXCOORD:      return "texcoord";
		case IQM_BLENDINDEXES:  return "blend indices";
		case IQM_BLENDWEIGHTS:  return "blend weights";
		case IQM_COLOR:         return "color";
		default:                return "unknown";
	}
}

static const char *iqm_va_format_str(uint32_t type) {
	switch(type) {
		case IQM_BYTE:      return "int8_t";
		case IQM_UBYTE:     return "uint8_t";
		case IQM_SHORT:     return "int16_t";
		case IQM_USHORT:    return "uint16_t";
		case IQM_INT:       return "int32_t";
		case IQM_UINT:      return "uint32_t";
		case IQM_HALF:      return "half";
		case IQM_FLOAT:     return "float";
		case IQM_DOUBLE:    return "double";
		default:            return "unknown";
	}
}

static bool read_fields(SDL_IOStream *rw, size_t n, uint32_t fields[n]) {
	size_t read_size = sizeof(fields[0]) * n;

	if(SDL_ReadIO(rw, fields, read_size) != read_size) {
		return false;
	}

	for(size_t i = 0; i < n; ++i) {
		fields[i] = SDL_Swap32LE(fields[i]);
	}

	return true;
}

static bool read_floats(SDL_IOStream *rw, size_t n, float floats[n]) {
	uint32_t fields[n];

	if(read_fields(rw, n, fields)) {
		memcpy(floats, fields, sizeof(fields[0]) * n);
		return true;
	}

	return false;
}

static bool iqm_read_header(const char *fpath, SDL_IOStream *rw, IQMHeader *hdr) {
	if(SDL_ReadIO(rw, &hdr->magic, sizeof(hdr->magic)) != sizeof(hdr->magic)) {
		if(SDL_GetIOStatus(rw) == SDL_IO_STATUS_EOF) {
			SDL_SetError("Premature EOF");
		}

		log_error("%s: read error: %s", fpath, SDL_GetError());
		return false;
	}

	if(memcmp(hdr->magic, IQM_MAGIC, sizeof(IQM_MAGIC))) {
		log_error("%s: not an IQM file (bad magic number)", fpath);
		return false;
	}

	if(!read_fields(rw, ARRAY_SIZE(hdr->u32_array), hdr->u32_array)) {
		log_error("%s: read error: %s", fpath, SDL_GetError());
		return false;
	}

	if(hdr->version != IQM_VERSION) {
		log_error("%s: unsupported IQM version (got %u, expected %u)", fpath, hdr->version, IQM_VERSION);
		return false;
	}

	if(hdr->num_meshes < 1) {
		log_error("%s: no meshes in model", fpath);
		return false;
	}

	if(hdr->num_meshes > MAX_MESHES) {
		log_error("%s too many meshes in model (%u allowed; have %u)", fpath, MAX_MESHES, hdr->num_meshes);
		return false;
	}

	if(hdr->num_vertexes < 1) {
		log_error("%s: no vertices in model", fpath);
		return false;
	}

	if(hdr->num_vertexes > MAX_VERTICES) {
		log_error("%s too many vertices in model (%u allowed; have %u)", fpath, MAX_VERTICES, hdr->num_vertexes);
		return false;
	}

	if(hdr->num_triangles < 1) {
		log_error("%s: no triangles in model", fpath);
		return false;
	}

	if(hdr->num_triangles > MAX_TRIANGLES) {
		log_error("%s too many triangles in model (%u allowed; have %u)", fpath, MAX_TRIANGLES, hdr->num_triangles);
		return false;
	}

	if(hdr->num_vertexarrays < NUM_REQUIRED_VERTEX_ARRAYS) {
		log_error("%s: not enough vertex arrays (%u expected; got %u)", fpath, NUM_REQUIRED_VERTEX_ARRAYS, hdr->num_vertexarrays);
		return false;
	}

	if(hdr->num_vertexarrays > MAX_VERTEX_ARRAYS) {
		log_error("%s too many vertex arrays in model (%u allowed; have %u)", fpath, MAX_VERTEX_ARRAYS, hdr->num_vertexarrays);
		return false;
	}

	log_debug("%s: IQM version %u; %u meshes; %u tris; %u vertices",
		fpath,
		hdr->version,
		hdr->num_meshes,
		hdr->num_triangles,
		hdr->num_vertexes
	);

	return true;
}

static bool iqm_read_meshes(const char *fpath, SDL_IOStream *rw,
			    uint num_meshes, IQMMesh meshes[num_meshes]) {
	for(uint i = 0; i < num_meshes; ++i) {
		if(!read_fields(rw, ARRAY_SIZE(meshes[i].u32_array), meshes[i].u32_array)) {
			log_error("%s: read error: %s", fpath, SDL_GetError());
			return false;
		}

		log_debug("Mesh #0: %u tris; %u vertices", meshes[i].num_triangles, meshes[i].num_vertexes);
	}

	return true;
}

static bool iqm_read_vertex_arrays(
	const char *fpath, SDL_IOStream *rw,
	uint num_varrs,
	IQMVertexArray varrs[num_varrs],
	VertexArrayIndices *indices
) {
	for(uint i = 0; i < num_varrs; ++i) {
		IQMVertexArray *va = varrs + i;

		if(!read_fields(rw, ARRAY_SIZE(va->u32_array), va->u32_array)) {
			log_error("%s: read error: %s", fpath, SDL_GetError());
			return false;
		}

		log_debug("Vertex array #%u: %s[%u] %s",
			i,
			iqm_va_format_str(va->format),
			va->size,
			iqm_va_type_str(va->type)
		);

		int *idx_p;
		uint32_t expected_size;

		switch(va->type) {
			case IQM_POSITION:
				idx_p = &indices->position;
				expected_size = 3;
				break;

			case IQM_TEXCOORD:
				idx_p = &indices->texcoord;
				expected_size = 2;
				break;

			case IQM_NORMAL:
				idx_p = &indices->normal;
				expected_size = 3;
				break;

			case IQM_TANGENT:
				idx_p = &indices->tangent;
				expected_size = 4;
				break;

			default:
				log_warn("%s: vertex array #%i ignored: unhandled type (%s)", fpath, i, iqm_va_type_str(va->type));
				continue;
		}

		if(*idx_p > 0) {
			log_warn("%s: vertex array #%i ignored: already using array #%i for %s data", fpath, i, *idx_p, iqm_va_type_str(va->type));
			continue;
		}

		if(va->size != expected_size) {
			log_warn("%s: vertex array #%i ignored: data size mismatch (expected %u; got %u)", fpath, i, expected_size, va->size);
			continue;
		}

		if(va->format != IQM_FLOAT) {
			log_warn("%s: vertex array #%i ignored: %s data type not supported", fpath, i, iqm_va_format_str(va->format));
			continue;
		}

		*idx_p = i;

		log_debug("Using vertex array #%u for %s data",
			i,
			iqm_va_type_str(va->type)
		);
	}

	bool ok = true;

	if(indices->position < 0) {
		log_error("%s: no position data", fpath);
		ok = false;
	}

	if(indices->texcoord < 0) {
		log_error("%s: no texcoord data", fpath);
		ok = false;
	}

	if(indices->normal < 0) {
		log_error("%s: no normal data", fpath);
		ok = false;
	}

	if(indices->tangent < 0) {
		log_error("%s: no tangent data", fpath);
		ok = false;
	}

	return ok;
}

static bool iqm_read_vert_positions(
	const char *fpath, SDL_IOStream *rw,
	uint num_verts,
	GenericModelVertex vertices[num_verts]
) {
	for(uint i = 0; i < num_verts; ++i) {
		if(!read_floats(rw, ARRAY_SIZE(vertices[i].position), vertices[i].position)) {
			log_error("%s: read error: %s", fpath, SDL_GetError());
			return false;
		}
	}

	return true;
}

static bool iqm_read_vert_texcoords(
	const char *fpath, SDL_IOStream *rw,
	uint num_verts,
	GenericModelVertex vertices[num_verts]
) {
	for(uint i = 0; i < num_verts; ++i) {
		if(!read_floats(rw, ARRAY_SIZE(vertices[i].uv), vertices[i].uv)) {
			log_error("%s: read error: %s", fpath, SDL_GetError());
			return false;
		}

		vertices[i].uv[1] = 1.0 - vertices[i].uv[1];
	}

	return true;
}

static bool iqm_read_vert_normals(
	const char *fpath, SDL_IOStream *rw,
	uint num_verts,
	GenericModelVertex vertices[num_verts]
) {
	for(uint i = 0; i < num_verts; ++i) {
		if(!read_floats(rw, ARRAY_SIZE(vertices[i].normal), vertices[i].normal)) {
			log_error("%s: read error: %s", fpath, SDL_GetError());
			return false;
		}
	}

	return true;
}

static bool iqm_read_vert_tangents(
	const char *fpath, SDL_IOStream *rw,
	uint num_verts,
	GenericModelVertex vertices[num_verts]
) {
	for(uint i = 0; i < num_verts; ++i) {
		if(!read_floats(rw, ARRAY_SIZE(vertices[i].tangent), vertices[i].tangent)) {
			log_error("%s: read error: %s", fpath, SDL_GetError());
			return false;
		}
	}

	return true;
}

static bool iqm_read_triangles(
	const char *fpath, SDL_IOStream *rw,
	uint num_tris, IQMTriangle triangles[num_tris]
) {
	if(!read_fields(rw, ARRAY_SIZE(triangles->u32_array) * num_tris, triangles->u32_array)) {
		log_error("%s: read error: %s", fpath, SDL_GetError());
		return false;
	}

	return true;
}

static bool range_is_valid(uint32_t total, uint32_t first, uint32_t num) {
	if(((uint64_t)first + (uint64_t)num) != (first + num)) {
		return false;
	}

	if(first + num > total) {
		return false;
	}

	return true;
}

bool iqm_load_stream(IQMModel *model, const char *filename, SDL_IOStream *stream) {
	IQMMesh *meshes = NULL;
	IQMVertexArray *vert_arrays = NULL;
	GenericModelVertex *vertices = NULL;
	union { uint32_t indices[3]; IQMTriangle tri; } *indices = NULL;
	bool success = true;

	#define TRY_SEEK(ofs) \
		do { \
			if(SDL_SeekIO(stream, ofs, SDL_IO_SEEK_SET) < 0) { \
				log_error("%s: %s", filename, SDL_GetError()); \
				goto fail; \
			} \
			assert(SDL_TellIO(stream) == ofs); \
		} while(0)

	#define TRY(...) \
		do { \
			if(!(__VA_ARGS__)) { \
				goto fail; \
			} \
		} while(0)

	IQMHeader hdr;
	TRY(iqm_read_header(filename, stream, &hdr));

	assume(hdr.num_meshes > 0);
	meshes = ALLOC_ARRAY(hdr.num_meshes, typeof(*meshes));

	TRY_SEEK(hdr.ofs_meshes);
	TRY(iqm_read_meshes(filename, stream, hdr.num_meshes, meshes));

	for(uint i = 0; i < hdr.num_meshes; ++i) {
		IQMMesh *mesh = meshes + i;

		if(!range_is_valid(hdr.num_vertexes, mesh->first_vertex, mesh->num_vertexes)) {
			log_error("%s: mesh %i: vertices out of range", filename, i);
			goto fail;
		}

		if(!range_is_valid(hdr.num_triangles, mesh->first_triangle, mesh->num_triangles)) {
			log_error("%s: mesh %i: triangles out of range", filename, i);
			goto fail;
		}
	}

	assume(hdr.num_vertexarrays > 0);
	vert_arrays = ALLOC_ARRAY(hdr.num_vertexarrays, typeof(*vert_arrays));

	TRY_SEEK(hdr.ofs_vertexarrays);
	VertexArrayIndices va_indices;
	for(uint i = 0; i < NUM_REQUIRED_VERTEX_ARRAYS; ++i) {
		va_indices.indices[i] = -1;
	}

	TRY(iqm_read_vertex_arrays(filename, stream, hdr.num_vertexarrays, vert_arrays, &va_indices));

	assume(hdr.num_vertexes > 0);
	vertices = ALLOC_ARRAY(hdr.num_vertexes, typeof(*vertices));

	TRY_SEEK(vert_arrays[va_indices.position].offset);
	TRY(iqm_read_vert_positions(filename, stream, hdr.num_vertexes, vertices));

	TRY_SEEK(vert_arrays[va_indices.texcoord].offset);
	TRY(iqm_read_vert_texcoords(filename, stream, hdr.num_vertexes, vertices));

	TRY_SEEK(vert_arrays[va_indices.normal].offset);
	TRY(iqm_read_vert_normals(filename, stream, hdr.num_vertexes, vertices));

	TRY_SEEK(vert_arrays[va_indices.tangent].offset);
	TRY(iqm_read_vert_tangents(filename, stream, hdr.num_vertexes, vertices));

	assume(hdr.num_triangles > 0);
	indices = ALLOC_ARRAY(hdr.num_triangles, typeof(*indices));

	TRY_SEEK(hdr.ofs_triangles);
	TRY(iqm_read_triangles(filename, stream, hdr.num_triangles, &indices->tri));

	*model = (IQMModel) {
		.vertices = vertices,
		.indices = indices->indices,
		.num_vertices = hdr.num_vertexes,
		.num_indices = hdr.num_triangles * 3,
	};

cleanup:
	mem_free(meshes);
	mem_free(vert_arrays);
	return success;

fail:
	mem_free(vertices);
	mem_free(indices);
	success = false;
	goto cleanup;
}
