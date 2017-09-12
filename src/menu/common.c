/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "global.h"
#include "menu.h"
#include "savereplay.h"
#include "difficultyselect.h"
#include "charselect.h"
#include "ending.h"
#include "credits.h"
#include "mainmenu.h"

static void start_game_internal(MenuData *menu, StageInfo *info, bool difficulty_menu) {
    MenuData m;
    Difficulty stagediff;
    bool restart;

    init_player(&global.plr);

    do {
        restart = false;
        stagediff = info ? info->difficulty : D_Any;

        if(stagediff == D_Any) {
            if(difficulty_menu) {
                create_difficulty_menu(&m);
                if(menu_loop(&m) == -1) {
                    return;
                }
            }
        } else {
            global.diff = stagediff;
        }

        create_char_menu(&m);
        if(menu_loop(&m) == -1) {
            if(stagediff != D_Any || !difficulty_menu) {
                return;
            }

            restart = true;
        }
    } while(restart);

    global.replay_stage = NULL;
    replay_init(&global.replay);

    int chr = global.plr.cha;
    int sht = global.plr.shot;

    do {
        restart = false;
        global.continues = 0;

        if(info) {
            global.is_practice_mode = (info->type != STAGE_EXTRA);
            stage_loop(info);
        } else {
            global.is_practice_mode = false;
            for(StageInfo *s = stages; s->type == STAGE_STORY; ++s) {
                stage_loop(s);
            }
        }

        if(global.game_over == GAMEOVER_RESTART) {
            init_player(&global.plr);
            replay_destroy(&global.replay);
            replay_init(&global.replay);
            global.game_over = 0;
            init_player(&global.plr);
            global.plr.cha  = chr;
            global.plr.shot = sht;

            restart = true;
        }
    } while(restart);

    free_resources(false);

    if(global.replay_stage) {
        switch(config_get_int(CONFIG_SAVE_RPY)) {
            case 0: break;

            case 1: {
                save_rpy(menu, NULL);
                break;
            }

            case 2: {
                MenuData m;
                create_saverpy_menu(&m);
                menu_loop(&m);
                break;
            }
        }

        global.replay_stage = NULL;
    }

    if(global.game_over == GAMEOVER_WIN && !info) {
        start_bgm("bgm_ending");
        ending_loop();
        start_bgm("bgm_credits");
        credits_loop();
        free_resources(false);
    }

    start_bgm("bgm_menu");
    replay_destroy(&global.replay);
    main_menu_update_practice_menus();
    global.game_over = 0;
}

void start_game(MenuData *m, void *arg) {
    start_game_internal(m, (StageInfo*)arg, true);
}

void start_game_no_difficulty_menu(MenuData *m, void *arg) {
    start_game_internal(m, (StageInfo*)arg, false);
}

void draw_menu_selector(float x, float y, float w, float h, float t) {
    Texture *bg = get_tex("part/smoke");
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(w / bg->w, h / bg->h, 1);
    glRotatef(t*2,0,0,1);
    glColor4f(0,0,0,0.5 * (1 - transition.fade));
    draw_texture_p(0,0,bg);
    glPopMatrix();
}

void draw_menu_title(MenuData *m, char *title) {
    glColor4f(1, 1, 1, 1);
    draw_text(AL_Right, (stringwidth(title, _fonts.mainmenu) + 10) * (1.0-menu_fade(m)), 30, title, _fonts.mainmenu);
}

void draw_menu_list(MenuData *m, float x, float y, void (*draw)(void*, int, int)) {
    glPushMatrix();
    float offset = ((((m->ecount+5) * 20) > SCREEN_H)? min(0, SCREEN_H * 0.7 - y - m->drawdata[2]) : 0);
    glTranslatef(x, y + offset, 0);

    draw_menu_selector(m->drawdata[0], m->drawdata[2], m->drawdata[1], 34, m->frames);

    int i;
    for(i = 0; i < m->ecount; i++) {
        MenuEntry *e = &(m->entries[i]);
        e->drawdata += 0.2 * (10*(i == m->cursor) - e->drawdata);

        float p = offset + 20*i;

        if(p < -y-10 || p > SCREEN_H+10)
            continue;

        float a = e->drawdata * 0.1;
        float o = (p < 0? 1-p/(-y-10) : 1);
        if(e->action == NULL)
            glColor4f(0.5, 0.5, 0.5, 0.5*o);
        else {
            float ia = 1-a;
            glColor4f(0.9 + ia * 0.1, 0.6 + ia * 0.4, 0.2 + ia * 0.8, (0.7 + 0.3 * a)*o);
        }

        if(draw && i < m->ecount-1)
            draw(e, i, m->ecount);
        else if(e->name)
            draw_text(AL_Left, 20 - e->drawdata, 20*i, e->name, _fonts.standard);
    }

    glPopMatrix();
}

void animate_menu_list(MenuData *m) {
    MenuEntry *s = m->entries + m->cursor;
    int w = stringwidth(s->name, _fonts.standard);

    m->drawdata[0] += (10 + w/2.0 - m->drawdata[0])/10.0;
    m->drawdata[1] += (w*2 - m->drawdata[1])/10.0;
    m->drawdata[2] += (20*m->cursor - m->drawdata[2])/10.0;
}

void menu_commonaction_close(MenuData *menu, void *arg) {
    menu->state = MS_Dead;
}
