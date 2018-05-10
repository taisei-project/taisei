
#define PP_BASIC(name, w, h, cw, ch) PP(name)
#include "basic.inc.h"

#define PP_PLAYER(name, w, h) PP(name)
#include "player.inc.h"

PP(blast)

#undef PP
