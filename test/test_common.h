
#include "log_sdl.h"
#include "taisei.h"

#include "log.h"
#include "rwops/rwops_stdiofp.h"
#include "events.h"

#include <locale.h>

static void test_init_log(void) {
	log_init(LOG_ALL);
	log_add_output(LOG_ALL, SDL_RWFromFP(stderr, false), log_formatter_console);
}

static void test_init_sdl(void) {
	mem_install_sdl_callbacks();

	if(!SDL_Init(SDL_INIT_EVENTS)) {
		log_fatal("SDL_Init() failed: %s", SDL_GetError());
	}

	events_init();
}

static void test_init_basic(void) {
	setlocale(LC_ALL, "C");
	mem_install_sdl_callbacks();
	thread_init();
	test_init_log();
	test_init_sdl();
	log_sdl_init(SDL_LOG_PRIORITY_VERBOSE);
}

static void test_shutdown_basic(void) {
	log_shutdown();
	events_shutdown();
	thread_shutdown();
	SDL_Quit();
}
