#ifndef OGG_H
#define OGG_H

#include <AL/al.h>

int load_ogg(char *filename, ALenum *format, char **buffer, ALsizei *size, ALsizei *freq);

#endif