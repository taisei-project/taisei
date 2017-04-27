
#ifndef RWZIP_H
#define RWZIP_H

#include <SDL.h>
#include <zip.h>
#include <stdbool.h>

SDL_RWops* SDL_RWFromZipFile(zip_file_t *zipfile, bool autoclose);

#endif
