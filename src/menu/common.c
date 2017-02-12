/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "global.h"
#include "menu.h"
#include "savereplay.h"
#include "difficulty.h"
#include "charselect.h"
#include "ending.h"
#include "credits.h"

void start_game(void *arg) {
    MenuData m;

    init_player(&global.plr);

troll:
    create_difficulty_menu(&m);
    if(difficulty_menu_loop(&m) == -1)
        return;

    create_char_menu(&m);
    if(char_menu_loop(&m) == -1)
        goto troll;

    global.replay_stage = NULL;
    replay_init(&global.replay);

    int chr = global.plr.cha;
    int sht = global.plr.shot;

troll2:
    if(arg)
        ((StageInfo*)arg)->loop();
    else {
        int i;
        for(i = 0; stages[i].loop; ++i)
            stages[i].loop();
    }

    if(global.game_over == GAMEOVER_RESTART) {
        init_player(&global.plr);
        replay_destroy(&global.replay);
        replay_init(&global.replay);
        global.game_over = 0;
        init_player(&global.plr);
        global.plr.cha  = chr;
        global.plr.shot = sht;
        goto troll2;
    }

    if(global.replay_stage) {
        switch(tconfig.intval[SAVE_RPY]) {
            case 0: break;

            case 1: {
                save_rpy(NULL);
                break;
            }

            case 2: {
                MenuData m;
                create_saverpy_menu(&m);
                saverpy_menu_loop(&m);
                break;
            }
        }

        global.replay_stage = NULL;
    }

    if(global.game_over == GAMEOVER_WIN && !arg) {
        start_bgm("bgm_ending");
        ending_loop();
        start_bgm("bgm_credits");
        credits_loop();
    }

    start_bgm("bgm_menu");
    replay_destroy(&global.replay);
    global.game_over = 0;
}

void draw_menu_selector(float x, float y, float w, float h, float t) {
    Texture *bg = get_tex("part/smoke");
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(w / bg->w, h / bg->h, 1);
    glRotatef(t*2,0,0,1);
    glColor4f(0,0,0,0.5);
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
