/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "test_common.h"

#include "memory/scratch.h"
#include "rwops/rwops_sha256.h"
#include "rwops/rwops_util.h"
#include "util/miscmath.h"
#include "util/sha256.h"
#include "util/stringops.h"
#include "vfs/public.h"

static const char *io_status_name(SDL_IOStatus status) {
	switch(status) {
		#define S(s) case s: return #s; break
		S(SDL_IO_STATUS_READY);
		S(SDL_IO_STATUS_ERROR);
		S(SDL_IO_STATUS_EOF);
		S(SDL_IO_STATUS_NOT_READY);
		S(SDL_IO_STATUS_READONLY);
		S(SDL_IO_STATUS_WRITEONLY);
		#undef S
	}

	return "???";
}

int main(int argc, char **argv) {
	test_init_basic();

	if(argc < 2) {
		log_fatal("Path to directory with *.zip and *.zip.sha256 files expected as first argument");
	}

	const char *zips_path = argv[1];
	vfs_init();

	if(!vfs_mount_syspath("zips", zips_path, VFS_SYSPATH_MOUNT_READONLY)) {
		log_fatal("Failed to mount '%s': %s", zips_path, vfs_get_error());
	}

	StringBuffer sbuf = { acquire_scratch_arena() };
	auto dir = vfs_dir_open("zips");

	int files_processed = 0;
	int errors = 0;

	for(const char *fname; (fname = vfs_dir_read(dir));) {
		if(!strendswith(fname, ".zip")) {
			continue;
		}

		strbuf_printf(&sbuf, "zips/%s", fname);
		if(!vfs_mount_zipfile("mnt", sbuf.start)) {
			log_fatal("Failed to mount %s: %s", vfs_repr(sbuf.start, true), vfs_get_error());
		}

		strbuf_cat(&sbuf, ".sha256");
		auto sha256list_io = vfs_open(sbuf.start, VFS_MODE_READ);

		if(!sha256list_io) {
			log_fatal("Failed to open %s: %s", vfs_repr(sbuf.start, true), vfs_get_error());
		}

		size_t sha256list_data_size;
		auto sha256list_data = SDL_LoadFile_IO(sha256list_io, &sha256list_data_size, true);

		if(!sha256list_data) {
			log_sdl_error(LOG_FATAL, "SDL_LoadFile_IO");
			abort();
		}

		char *save = sha256list_data;
		for(char *line; (line = strtok_r(NULL, "\n", &save)); ++files_processed) {
			if(strlen(line) < SHA256_HEXDIGEST_SIZE) {
				log_fatal("Malformed line in sha256list:\n%s", line);
			}

			line[SHA256_HEXDIGEST_SIZE - 1] = 0;
			char *hexdigest_from_list = line;
			char *filename = line + SHA256_HEXDIGEST_SIZE;

			strbuf_clear(&sbuf);
			strbuf_printf(&sbuf, "mnt/%s", filename);
			const char *inner_filename = strbuf_commit(&sbuf);

			auto file_io = vfs_open(inner_filename, VFS_MODE_READ);

			if(!file_io) {
				log_fatal("Failed to open %s: %s", vfs_repr(inner_filename, true), vfs_get_error());
			}

			auto sha256 = sha256_new();
			int64_t data_size = SDL_GetIOSize(file_io);

			if(data_size < 0) {
				++errors;
				log_error("%s: SDL_GetIOSize() failed: %s", iostream_get_name(file_io), SDL_GetError());
				goto skip_file;
			}

			int64_t real_pos, seek_pos;

			if(data_size > 8) {
				SDL_ReadU64LE(file_io, NULL);
				auto io_status = SDL_GetIOStatus(file_io);

				if(io_status == SDL_IO_STATUS_ERROR) {
					++errors;
					log_error("%s: IO error: %s", iostream_get_name(file_io), SDL_GetError());
					goto skip_file;
				} else if(io_status != SDL_IO_STATUS_READY) {
					++errors;
					log_error("%s: IO status is %s after read, must be SDL_IO_STATUS_READY",
						iostream_get_name(file_io), io_status_name(io_status));
				}

				// Make sure some seeking won't break it
				seek_pos = data_size / 4;
				real_pos = SDL_SeekIO(file_io, seek_pos, SDL_IO_SEEK_SET);

				if(real_pos < 0) {
					++errors;
					log_error("%s: SDL_SeekIO() failed: %s", iostream_get_name(file_io), SDL_GetError());
					goto skip_file;
				}

				if(real_pos != seek_pos) {
					++errors;
					log_error("%s: unexpected position %llu after seek, expected %llu",
						iostream_get_name(file_io), (unsigned long long)real_pos, (unsigned long long)seek_pos);
					goto skip_file;
				}

				SDL_ReadU64LE(file_io, NULL);
				io_status = SDL_GetIOStatus(file_io);

				if(io_status == SDL_IO_STATUS_ERROR) {
					++errors;
					log_error("%s: IO error: %s", iostream_get_name(file_io), SDL_GetError());
					goto skip_file;
				} else if(io_status != SDL_IO_STATUS_READY) {
					++errors;
					log_error("%s: IO status is %s after read, must be SDL_IO_STATUS_READY",
						iostream_get_name(file_io), io_status_name(io_status));
				}
			}

			seek_pos = 0;
			real_pos = SDL_SeekIO(file_io, seek_pos, SDL_IO_SEEK_SET);

			if(real_pos < 0) {
				++errors;
				log_error("%s: SDL_SeekIO() failed: %s", iostream_get_name(file_io), SDL_GetError());
				goto skip_file;
			}

			if(real_pos != seek_pos) {
				++errors;
				log_error("%s: unexpected position %llu after seek, expected %llu",
					iostream_get_name(file_io), (unsigned long long)real_pos, (unsigned long long)seek_pos);
				goto skip_file;
			}

			uint8_t chunk[256];
			size_t chunk_size = clamp(data_size / 8, 8, countof(chunk));
			size_t read_total = 0;

			file_io = SDL_RWWrapSHA256(file_io, sha256, true);

			bool was_eof = false;
			for(;;) {
				size_t read_size = SDL_ReadIO(file_io, chunk, chunk_size);
				read_total += read_size;

				auto io_status = SDL_GetIOStatus(file_io);

				if(io_status == SDL_IO_STATUS_ERROR) {
					++errors;
					log_error("%s: IO error after reading %llu bytes: %s",
						iostream_get_name(file_io), (unsigned long long)read_total, SDL_GetError());
					goto skip_file;
				}

				if(read_size == 0) {
					if(io_status != SDL_IO_STATUS_EOF) {
						++errors;
						log_error("%s: IO status is %s after 0-size read, must be SDL_IO_STATUS_EOF",
							iostream_get_name(file_io), io_status_name(io_status));
					}

					break;
				} else if(was_eof) {
					++errors;
					log_error("%s: non-0 read of size %llu after EOF, status is %s",
						iostream_get_name(file_io), (unsigned long long)read_size, io_status_name(io_status));
				} else if(io_status == SDL_IO_STATUS_EOF) {
					was_eof = true;
				}
			}

			if(read_total != data_size) {
				log_error("%s: read %llu bytes, but expected %llu",
					iostream_get_name(file_io), (unsigned long long)read_total, (unsigned long long)data_size);
			}

			uint8_t digest_computed[SHA256_BLOCK_SIZE];
			sha256_final(sha256, digest_computed, sizeof(digest_computed));

			char hexdigest_computed[SHA256_HEXDIGEST_SIZE];
			hexdigest(digest_computed, sizeof(digest_computed), hexdigest_computed, sizeof(hexdigest_computed));

			if(memcmp(hexdigest_computed, hexdigest_from_list, sizeof(hexdigest_computed))) {
				++errors;
				log_error("%s (size %llu) hashed to %s; expected %s",
					iostream_get_name(file_io), (unsigned long long)data_size, hexdigest_computed, hexdigest_from_list);
			}

skip_file:
			SDL_CloseIO(file_io);
			sha256_free(sha256);
			marena_reset(sbuf.arena);
			strbuf_clear(&sbuf);
		}

		SDL_free(sha256list_data);
		strbuf_clear(&sbuf);
		vfs_unmount("mnt");
	}

	vfs_dir_close(dir);
	release_scratch_arena(sbuf.arena);

	vfs_shutdown();

	if(errors) {
		log_fatal("%i errors found", errors);
	}

	if(!files_processed) {
		log_fatal("There were no files to process");
	}

	test_shutdown_basic();
	return 0;
}
