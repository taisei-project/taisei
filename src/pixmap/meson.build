
pixmap_src = files(
    'pixmap.c',
    'conversion.c',
)

pixmap_deps = [taisei_iostream_dep]

subdir('fileformats')
pixmap_src += pixmap_fileformats_src

# TODO: move
pixmap_game_src = files('pixmap_vfs.c')
