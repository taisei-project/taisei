
#ifndef RWDUMMY_H
#define RWDUMMY_H

#include <SDL.h>
#include <stdbool.h>

SDL_RWops* SDL_RWWrapDummy(SDL_RWops *src, bool autoclose);

#endif
