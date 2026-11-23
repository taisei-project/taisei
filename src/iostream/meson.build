
iostream_deps = [
    taisei_base_dep,
    dep_zlib,
    dep_zstd,
]

iostream_src = files(
    'iostream_arena.c',
    'iostream_crc32.c',
    'iostream_dummy.c',
    'iostream_segment.c',
    'iostream_sha256.c',
    'iostream_stdiofp.c',
    'iostream_util.c',
    'iostream_zlib.c',
    'iostream_zstd.c',
)

if is_developer_build
    iostream_src += files(
        'iostream_trace.c',
    )
endif
