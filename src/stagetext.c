/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "stagetext.h"
#include "list.h"
#include "global.h"

static StageText *textlist = NULL;

StageText* stagetext_add(const char *text, complex pos, Alignment align, TTF_Font *font, Color clr, int delay, int lifetime, int fadeintime, int fadeouttime) {
    StageText *t = (StageText*)list_append((List**)&textlist, malloc(sizeof(StageText)));
    t->rendered_text = fontrender_render(&resources.fontren, text, font);
    t->pos = pos;
    t->align = align;

    t->time.spawn = global.frames + delay;
    t->time.life = lifetime + fadeouttime;
    t->time.fadein = fadeintime;
    t->time.fadeout = fadeouttime;

    parse_color_array(clr, t->clr);
    memset(&t->custom, 0, sizeof(t->custom));

    return t;
}

void stagetext_numeric_predraw(StageText *txt, int t, float a) {
    char buf[32];
    SDL_FreeSurface(txt->rendered_text);
    snprintf(buf, sizeof(buf), "%i", (int)((intptr_t)txt->custom.data1 * pow(a, 5)));
    txt->rendered_text = fontrender_render(&resources.fontren, buf, *(TTF_Font**)txt->custom.data2);
}

StageText* stagetext_add_numeric(int n, complex pos, Alignment align, TTF_Font **font, Color clr, int delay, int lifetime, int fadeintime, int fadeouttime) {
    StageText *t = stagetext_add("0", pos, align, *font, clr, delay, lifetime, fadeintime, fadeouttime);
    t->custom.data1 = (void*)(intptr_t)n;
    t->custom.data2 = (void*)font;
    t->custom.predraw = stagetext_numeric_predraw;
    return t;
}

static void* stagetext_delete(List **dest, List *txt, void *arg) {
    SDL_FreeSurface(((StageText*)txt)->rendered_text);
    free(list_unlink(dest, txt));
    return NULL;
}

void stagetext_free(void) {
    list_foreach((List**)&textlist, stagetext_delete, NULL);
}

static void stagetext_draw_single(StageText *txt) {
    if(global.frames < txt->time.spawn) {
        return;
    }

    if(global.frames > txt->time.spawn + txt->time.life) {
        stagetext_delete((List**)&textlist, (List*)txt, NULL);
        return;
    }

    int t = global.frames - txt->time.spawn;
    float f = 1.0 - clamp((txt->time.life - t) / (float)txt->time.fadeout, 0, clamp(t / (float)txt->time.fadein, 0, 1));

    if(txt->custom.predraw) {
        txt->custom.predraw(txt, t, 1.0 - f);
    }

    Shader *sha = get_shader("stagetitle");
    glUseProgram(sha->prog);
    glUniform1i(uniloc(sha, "trans"), 1);
    glUniform1f(uniloc(sha, "t"), 1.0 - f);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, get_tex("titletransition")->gltex);
    glActiveTexture(GL_TEXTURE0);

    glUniform3f(uniloc(sha, "color"), 0,0,0);
    draw_text_prerendered(txt->align, creal(txt->pos)+10*f*f+1, cimag(txt->pos)+10*f*f+1, txt->rendered_text);
    glUniform3fv(uniloc(sha, "color"), 1, txt->clr);
    draw_text_prerendered(txt->align, creal(txt->pos)+10*f*f, cimag(txt->pos)+10*f*f, txt->rendered_text);

    glUseProgram(0);
}

void stagetext_draw(void) {
    for(StageText *t = textlist, *next = NULL; t; t = next) {
        next = t->next;
        stagetext_draw_single(t);
    }
}

static void stagetext_table_push(StageTextTable *tbl, StageText *txt, bool update_pos) {
    list_append(&tbl->elems, list_wrap_container(txt));

    if(update_pos) {
        tbl->pos += txt->rendered_text->h / resources.fontren.quality * I;
    }

    tbl->delay += 5;
}

void stagetext_begin_table(StageTextTable *tbl, const char *title, Color titleclr, Color clr, double width, int delay, int lifetime, int fadeintime, int fadeouttime) {
    memset(tbl, 0, sizeof(StageTextTable));
    tbl->pos = VIEWPORT_W/2 + VIEWPORT_H/2*I;
    tbl->clr = clr;
    tbl->width = width;
    tbl->lifetime = lifetime;
    tbl->fadeintime = fadeintime;
    tbl->fadeouttime = fadeouttime;
    tbl->delay = delay;

    StageText *txt = stagetext_add(title, tbl->pos, AL_Center, _fonts.mainmenu, titleclr, tbl->delay, lifetime, fadeintime, fadeouttime);
    stagetext_table_push(tbl, txt, true);
}

void stagetext_end_table(StageTextTable *tbl) {
    complex ofs = -0.5 * I * (cimag(tbl->pos) - VIEWPORT_H/2);

    for(ListContainer *c = tbl->elems; c; c = c->next) {
        ((StageText*)c->data)->pos += ofs;
    }

    list_free_all(&tbl->elems);
}

static void stagetext_table_add_label(StageTextTable *tbl, const char *title) {
    StageText *txt = stagetext_add(title, tbl->pos - tbl->width * 0.5, AL_Left, _fonts.standard, tbl->clr, tbl->delay, tbl->lifetime, tbl->fadeintime, tbl->fadeouttime);
    stagetext_table_push(tbl, txt, false);
}

void stagetext_table_add(StageTextTable *tbl, const char *title, const char *val) {
    stagetext_table_add_label(tbl, title);
    StageText *txt = stagetext_add(val, tbl->pos + tbl->width * 0.5, AL_Right, _fonts.standard, tbl->clr, tbl->delay, tbl->lifetime, tbl->fadeintime, tbl->fadeouttime);
    stagetext_table_push(tbl, txt, true);
}

void stagetext_table_add_numeric(StageTextTable *tbl, const char *title, int n) {
    stagetext_table_add_label(tbl, title);
    StageText *txt = stagetext_add_numeric(n, tbl->pos + tbl->width * 0.5, AL_Right, &_fonts.standard, tbl->clr, tbl->delay, tbl->lifetime, tbl->fadeintime, tbl->fadeouttime);
    stagetext_table_push(tbl, txt, true);
}

void stagetext_table_add_numeric_nonzero(StageTextTable *tbl, const char *title, int n) {
    if(n) {
        stagetext_table_add_numeric(tbl, title, n);
    }
}

void stagetext_table_add_separator(StageTextTable *tbl) {
    tbl->pos += I * 0.5 * stringheight("Love Live", _fonts.standard);
}

void stagetext_table_test(void) {
    StageTextTable tbl;
    stagetext_begin_table(&tbl, "Test", rgb(1, 1, 1), rgb(1, 1, 1), VIEWPORT_W/2, 60, 300, 30, 60);
    stagetext_table_add(&tbl, "foo", "bar");
    stagetext_table_add(&tbl, "qwerty", "asdfg");
    stagetext_table_add(&tbl, "top", "kek");
    stagetext_table_add_separator(&tbl);
    stagetext_table_add_numeric(&tbl, "Total Score", 9000000);
    stagetext_end_table(&tbl);
}
