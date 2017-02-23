
#ifndef ZWROPS_H
#define ZWROPS_H

#include <SDL.h>
#include <zlib.h>

SDL_RWops* SDL_RWWrapZReader(SDL_RWops *src, size_t bufsize);
SDL_RWops* SDL_RWWrapZWriter(SDL_RWops *src, size_t bufsize);
z_stream* SDL_RWGetZStream(SDL_RWops *src);

int zrwops_test(void);

#endif
