// Based on "Introduction to Ogg Vorbis" example code
// by Anthony "TangentZ" Yuen
//
// https://www.gamedev.net/resources/_/technical/game-programming/introduction-to-ogg-vorbis-r2031 

#include <AL/al.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/vorbisfile.h>

inline int detect_endianness(void)
{
    // Returns 0 for little endian, 1 for big endian.
    union { uint16_t patterns; uint8_t endianness; } test_endianness = { .patterns = 0x0100 };
    return test_endianness.endianness;
}

#define BUFFER_SIZE     32768       // 32 KB buffers

int load_ogg(char *filename, ALenum *format, char **buffer, ALsizei *size, ALsizei *freq)
{
	FILE *f;
	f = fopen(filename, "rb");
	if (f == NULL) return -1; // Error opening for reading
	
	OggVorbis_File oggFile;
	if (ov_open(f, &oggFile, NULL, 0) != 0)
	{
		fclose(f);
		return -2; // Error opening for decoding
	}
	
	vorbis_info *pInfo;
	pInfo = ov_info(&oggFile, -1);
	*format = (pInfo->channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	*freq = pInfo->rate;
	
	int bitstream;
	long bytes;
	char array[BUFFER_SIZE];
	*buffer = NULL; *size = 0;
	do
	{
		bytes = ov_read(&oggFile, array, BUFFER_SIZE, detect_endianness(), 2, 1, &bitstream);
		if (bytes < 0)
		{
			fclose(f);
			ov_clear(&oggFile);
			return -3; // Error decoding
		}
		
		*buffer = realloc(*buffer, *size + bytes);
		if(*buffer == NULL)
		{
			fclose(f);
			ov_clear(&oggFile);
			return -4; // Memory limit exceeded
		}
		
		memcpy(*buffer + *size, array, bytes);
		*size += bytes;
	}
	while (bytes > 0);
	
	ov_clear(&oggFile);
	return 0;
}
