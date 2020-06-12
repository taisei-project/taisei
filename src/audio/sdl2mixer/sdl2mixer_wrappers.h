

static int wrap_Mix_SetMusicPosition(double pos) {
	int error = Mix_SetMusicPosition(pos);
	if(error) {
		log_error("Mix_SetMusicPosition() failed: %s", Mix_GetError());
	}
	return error;
}
#define Mix_SetMusicPosition wrap_Mix_SetMusicPosition

static double wrap_Mix_GetMusicPosition(Mix_Music *mus) {
	double pos = Mix_GetMusicPosition(mus);
	if(pos < 0) {
		log_error("Mix_GetMusicPosition() failed: %s", Mix_GetError());
	}
	return pos;
}
#define Mix_GetMusicPosition wrap_Mix_GetMusicPosition

static double wrap_Mix_MusicDuration(Mix_Music *mus) {
	double duration = Mix_MusicDuration(mus);
	if(duration < 0) {
		log_error("Mix_MusicDuration() failed: %s", Mix_GetError());
	}
	return duration;
}
#define Mix_MusicDuration wrap_Mix_MusicDuration

static int wrap_Mix_PlayMusic(Mix_Music *mus, int loops) {
	int error = Mix_PlayMusic(mus, loops);
	if(error) {
		log_error("Mix_PlayMusic() failed: %s", Mix_GetError());
	}
	return error;
}
#define Mix_PlayMusic wrap_Mix_PlayMusic

static int wrap_Mix_FadeInMusicPos(Mix_Music *music, int loops, int ms, double position) {
	int error = Mix_FadeInMusicPos(music, loops, ms, position);
	if(error) {
		log_error("Mix_PlayMusic() failed: %s", Mix_GetError());
	}
	return error;
}
#define Mix_FadeInMusicPos wrap_Mix_FadeInMusicPos
