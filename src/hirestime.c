/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "util.h"
#include "hirestime.h"

static bool use_hires;
static hrtime_t time_current;
static hrtime_t time_offset;
static uint64_t prev_hires_time;
static uint64_t prev_hires_freq;
static SDL_mutex *paranoia;

static void time_update(void) {
    bool retry;

    do {
        retry = false;

        uint64_t freq = SDL_GetPerformanceFrequency();
        uint64_t cntr = SDL_GetPerformanceCounter();

        if(freq != prev_hires_freq) {
            log_debug("High resolution timer frequency changed: was %"PRIu64", now %"PRIu64". Saved time offset: %.16Lf", prev_hires_freq, freq, time_offset);
            time_offset = time_current;
            prev_hires_freq = freq;
            prev_hires_time = SDL_GetPerformanceCounter();
            retry = true;
            continue;
        }

        hrtime_t time_new = time_offset + (hrtime_t)(cntr - prev_hires_time) / freq;

        if(time_new < time_current) {
            log_warn("BUG: time went backwards. Was %.16Lf, now %.16Lf. Possible cause: your OS sucks spherical objects. Attempting to correct this...", time_current, time_new);
            time_offset = time_current;
            time_current = 0;
            prev_hires_time = SDL_GetPerformanceCounter();
            retry = true;
        } else {
            time_current = time_new;
        }
    } while(retry);
}

void time_init(void) {
    use_hires = getenvint("TAISEI_HIRES_TIMER", 1);

    if(use_hires) {
        if(!(paranoia = SDL_CreateMutex())) {
            log_warn("Not using the system high resolution timer: SDL_CreateMutex() failed: %s", SDL_GetError());
            return;
        }

        log_info("Using the system high resolution timer");
        prev_hires_time = SDL_GetPerformanceCounter();
        prev_hires_freq = SDL_GetPerformanceFrequency();
    } else {
        log_info("Not using the system high resolution timer: disabled by environment");
        return;
    }
}

void time_shutdown(void) {
    if(paranoia) {
        SDL_DestroyMutex(paranoia);
        paranoia = NULL;
    }
}

hrtime_t time_get(void) {
    if(use_hires) {
        SDL_LockMutex(paranoia);
        time_update();
        hrtime_t t = time_current;
        SDL_UnlockMutex(paranoia);
        return t;
    }

    return SDL_GetTicks() / 1000.0;
}
