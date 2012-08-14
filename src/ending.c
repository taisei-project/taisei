/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "ending.h"
#include "global.h"
#include "credits.h"

void add_ending_entry(Ending *e, int dur, char *msg, char *tex) {
	EndingEntry *entry;	
	e->entries = realloc(e->entries, (++e->count)*sizeof(EndingEntry));
	entry = &e->entries[e->count-1];
	
	entry->time = e->duration;
	e->duration += dur;
	entry->msg = strdup(msg);
	
	if(tex)
		entry->tex = get_tex(tex);
	else
		entry->tex = NULL;
}

void bad_ending_marisa(Ending *e) {
	add_ending_entry(e, 300, "After her consciousness had faded while she was falling down the tower,\nMarisa found herself waking up in a clearing of the magical forest.", NULL);
	add_ending_entry(e, 300, "She saw the sun set.", NULL);
	add_ending_entry(e, 300, "Maybe all of this was just a day dream.", NULL);
	add_ending_entry(e, 300, "But nevertheless, she won the fight, that's all what counts... isn't it?", NULL);
	add_ending_entry(e, 200, "[Bad Ending 1]", NULL);
}

void bad_ending_youmu(Ending *e) {
	add_ending_entry(e, 400, "After getting unconscious from falling the tower,\nYoumu only remembered how she rushed back to Hakugyokuro.", NULL);
	add_ending_entry(e, 300, "The anomalies were gone, everything went back to normal.", NULL);
	add_ending_entry(e, 400, "Youmu was relieved, but she felt that the real mystery of the land\nbehind the tunnel was left unsolved forever.", NULL);
	add_ending_entry(e, 300, "This left her unsatisfied...", NULL);
	add_ending_entry(e, 200, "[Bad Ending 2]", NULL);
}

void good_ending_marisa(Ending *e) {
	add_ending_entry(e, 400, "As soon as Elly was defeated, the room they were in\nbegan to fade and they landed softly on a wide plain of grass.", NULL);
	add_ending_entry(e, 350, "Elly and her friend promised that they won't cause any more trouble,\nbut Marisa was curious to explore the rest of this unknown land.", NULL);
	add_ending_entry(e, 350, "Who was the real culprit? What were their motives?", NULL);
	add_ending_entry(e, 350, "She craved to find out...", NULL);
	add_ending_entry(e, 200, "[Good Ending 1]", NULL);
}

void good_ending_youmu(Ending *e) {
	add_ending_entry(e, 300, "When they reached the ground, the tower and everything in that world\nbegan to fade to a endless plain of grass.", NULL);
	add_ending_entry(e, 550, "\"Always consider what trouble you cause others when you plan a mega project like that\"\nsaid Youmu confidently,\n\"that's the first rule in Gensokyo, if you don't want people to come for you and fight.\"", NULL);
	add_ending_entry(e, 320, "Elly would fix up the the border as soon as possible as she promised.", NULL);
	add_ending_entry(e, 350, "But before the way to this unknown place was sealed forever,\nYoumu decided travel it once more...", NULL);
	add_ending_entry(e, 200, "[Good Ending 2]", NULL);
}

void create_ending(Ending *e) {
	memset(e, 0, sizeof(Ending));
	
	if(global.plr.continues || global.diff == D_Easy) {
		switch(global.plr.cha) {
		case Marisa:
			bad_ending_marisa(e);
			break;
		case Youmu:
			bad_ending_youmu(e);
			break;
		}
		
		add_ending_entry(e, 400, "Try a no continue run on higher difficulties. You can do it!", NULL);
	} else {
		switch(global.plr.cha) {
		case Marisa:
			good_ending_marisa(e);
			break;
		case Youmu:
			good_ending_youmu(e);
			break;
		}
		add_ending_entry(e, 400, "Sorry, extra stage isn't done yet. ^^", NULL);
	}
	
	if(global.diff == D_Lunatic)
		add_ending_entry(e, 400, "Lunatic? Didn't know this was even possible. Cheater.", NULL);
	
	add_ending_entry(e, 400, "", NULL);
}	
	
void free_ending(Ending *e) {
	int i;
	for(i = 0; i < e->count; i++)
		free(e->entries[i].msg);
	free(e->entries);
}

void ending_draw(Ending *e) {
	float s, d;
	int t1 = global.frames-e->entries[e->pos].time;
	int t2 = e->entries[e->pos+1].time-global.frames;
		
	d = 1.0/ENDING_FADE_TIME;
	
	if(t1 < ENDING_FADE_TIME)
		s = clamp(d*t1, 0.0, 1.0);
	else
		s = clamp(d*t2, 0.0, 1.0);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glColor4f(1,1,1,s);
	if(e->entries[e->pos].tex)
		draw_texture_p(SCREEN_W/2, SCREEN_H/2, e->entries[e->pos].tex);
	
	draw_text(AL_Center, SCREEN_W/2, SCREEN_H/5*4, e->entries[e->pos].msg, _fonts.standard);
	glColor4f(1,1,1,1);
	
	draw_transition();
}

void ending_loop(void) {
	Ending e;
	create_ending(&e);
	
	global.frames = 0;
	set_ortho();
		
	while(e.pos < e.count-1) {
		credits_input();
		
		ending_draw(&e);
		global.frames++;
		SDL_GL_SwapBuffers();
		frame_rate(&global.lasttime);
		
		if(global.frames >= e.entries[e.pos+1].time)
			e.pos++;
		
		if(global.frames == e.entries[e.count-1].time-ENDING_FADE_OUT)
			set_transition(TransFadeWhite, ENDING_FADE_OUT, ENDING_FADE_OUT);
	}
}