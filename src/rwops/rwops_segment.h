
#ifndef RWSEGMENT_H
#define RWSEGMENT_H

#include <SDL.h>
#include <stdbool.h>

SDL_RWops* SDL_RWWrapSegment(SDL_RWops *src, size_t start, size_t end, bool autoclose);

#endif
