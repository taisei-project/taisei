
#ifndef RWAUTO_H
#define RWAUTO_H

#include <SDL.h>

SDL_RWops* SDL_RWAutoBuffer(void **ptr, size_t initsize);

#endif
