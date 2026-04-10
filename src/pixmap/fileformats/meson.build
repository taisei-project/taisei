
pixmap_fileformats_src = files(
    'bmp.c',
    'internal.c',
    'webp.c',
)

if dep_png.found()
    pixmap_fileformats_src += files('png.c')
else
    pixmap_fileformats_src += files('png_sdl3.c')
endif
