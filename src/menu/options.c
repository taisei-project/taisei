
#include "menu.h"
#include "options.h"
#include "global.h"

void do_nothing(void *arg) { }
void backtomain(void *arg)
{
	MenuData *m = arg;
	m->quit = 2;
}

void allocate_bindings(MenuData *m)
{
	OptionBinding *binds = malloc(m->ecount * sizeof(OptionBinding));
	int i;
	
	for(i = 0; i < m->ecount; ++i)
	{
		binds[i].values 	 = NULL;
		binds[i].getter		 = NULL;
		binds[i].setter 	 = NULL;
		binds[i].selected 	 = -1;
		binds[i].valcount	 = 0;
		binds[i].configentry = -1;
		binds[i].enabled	 = False;
	}
	
	m->context = (void*)binds;
}

void free_bindings(void *menu)
{
	MenuData *m = (MenuData*)menu;
	OptionBinding *binds = (OptionBinding*)m->context;
	
	int i, j;
	for(i = 0; i < m->ecount; ++i)
	{
		OptionBinding *bind = &(binds[i]);
		if(!bind->enabled)
			continue;
		
		if(bind->values) for(j = 0; j < bind->valcount; ++j)
			free(bind->values[j]);
		free(bind->optname);
	}
	free(binds);
}

OptionBinding* bind_option_to_entry(MenuData *m, int entry, char *optname, int cfgentry, BindingGetter getter, BindingSetter setter)
{
	OptionBinding *binds = (OptionBinding*)m->context;
	OptionBinding *bind = &(binds[entry]);
	
	bind->getter = getter;
	bind->setter = setter;
	bind->configentry = cfgentry;
	bind->optname = malloc((strlen(optname) + 1) * sizeof(char));
	strcpy(bind->optname, optname);
	bind->enabled = True;
	
	return bind;
}

int bind_addvalue(OptionBinding *b, char *val)
{
	if(!b->values)
		b->values = malloc(++b->valcount * sizeof(char));
	else
		b->values = realloc(b->values, ++b->valcount * sizeof(char));
	
	b->values[b->valcount-1] = malloc((strlen(val) + 1) * sizeof(char));
	strcpy(b->values[b->valcount-1], val);
	return b->valcount-1;
}

int binding_setvalue(OptionBinding *b, int v)
{
	return b->selected = b->setter(b, v);
}

int binding_setnext(OptionBinding *b)
{
	int s = b->selected + 1;
	if(s >= b->valcount)
		s = 0;
		
	return binding_setvalue(b, s);
}

int binding_setprev(OptionBinding *b)
{
	int s = b->selected - 1;
	if(s < 0)
		s = b->valcount - 1;
		
	return binding_setvalue(b, s);
}

void bindings_initvalues(MenuData *m)
{
	OptionBinding *binds = (OptionBinding*)m->context;
	
	int i;
	for(i = 0; i < m->ecount; ++i)
		if(binds[i].enabled)
			binds[i].selected = binds[i].getter(&(binds[i]));
}

int bind_common_onoffget(void *b)
{
	return !tconfig.intval[((OptionBinding*)b)->configentry];
}

int bind_common_onoffset(void *b, int v)
{
	return !(tconfig.intval[((OptionBinding*)b)->configentry] = !v);
}

int bind_common_onoffget_inverted(void *b)
{
	return tconfig.intval[((OptionBinding*)b)->configentry];
}

int bind_common_onoffset_inverted(void *b, int v)
{
	return tconfig.intval[((OptionBinding*)b)->configentry] = v;
}

int bind_fullscreen_set(void *b, int v)
{
	toggle_fullscreen();
	return bind_common_onoffset(b, v);
}

int bind_noaudio_set(void *b, int v)
{
	int i = bind_common_onoffset_inverted(b, v);
	
	if(!v)
	{
		init_alut();
		load_resources();
	}
	
	return i;
}

int bind_noshader_set(void *b, int v)
{
	int i = bind_common_onoffset_inverted(b, v);
	
	if(!v)
	{
		_init_fbo();
		load_resources();
	}
	
	return i;
}

#define bind_onoff(b) bind_addvalue(b, "on"); bind_addvalue(b, "off")

void create_options_menu(MenuData *m) {
	OptionBinding* b;
	
	create_menu(m);
	m->type = MT_Persistent;
	m->ondestroy = free_bindings;
	
	add_menu_entry(m, "Fullscreen", do_nothing, NULL);
	add_menu_entry(m, "Audio", do_nothing, NULL);
	add_menu_entry(m, "Shader", do_nothing, NULL);
	add_menu_entry(m, " ", NULL, NULL);
	add_menu_entry(m, "Customize controls", NULL, NULL);
	add_menu_entry(m, "Back", backtomain, m);
	
	allocate_bindings(m);
	
	b = bind_option_to_entry(m, 0, "fullscreen", FULLSCREEN, bind_common_onoffget, bind_fullscreen_set);
		bind_onoff(b);
	
	b = bind_option_to_entry(m, 1, "disable_audio", NO_AUDIO, bind_common_onoffget_inverted,
															  bind_noaudio_set);
		bind_onoff(b);
	
	b = bind_option_to_entry(m, 2, "disable_shader", NO_SHADER, bind_common_onoffget_inverted,
																bind_noshader_set);
		bind_onoff(b);
	
	bindings_initvalues(m);
}

void draw_options_menu_bg(MenuData* menu) {
	glColor4f(0.3, 0.3, 0.3, 1);
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubgbg");
	glColor4f(1,0.6,0.5,0.6 + 0.1*sin(menu->frames/100.0));
	draw_texture(SCREEN_W/2, SCREEN_H/2, "mainmenu/mainmenubg");
}

void draw_options_menu(MenuData *menu) {
	draw_options_menu_bg(menu);
	glColor4f(1,1,1,1);
	draw_text(AL_Right, 140*(1-menu->fade), 30, "Options", _fonts.mainmenu);
	
	//glColor4f(1,1,1,0.7);
	//draw_texture(SCREEN_W/2+40, SCREEN_H/2, "mainmenu/gate");
	
	glPushMatrix();
	glTranslatef(100, 100, 0);
	
	glPushMatrix();
	glTranslatef(0, menu->drawdata[2], 0);
	glColor4f(0,0,0,0.5);
	glBegin(GL_QUADS);
		glVertex3f(0, -10, 0);
		glVertex3f(0,  10, 0);
		glVertex3f(SCREEN_W - 200,  10, 0);
		glVertex3f(SCREEN_W - 200, -10, 0);
	glEnd();
	glPopMatrix();
	
	OptionBinding *binds = (OptionBinding*)menu->context;
	OptionBinding *bind;
	int i;

	for(i = 0; i < menu->ecount; i++) {
		float s = 0;
		
		if(menu->entries[i].action == NULL)
			glColor4f(0.5, 0.5, 0.5, 0.7);
		else if(i == menu->cursor)
		{
			glColor4f(1,1,0,0.7);
			s = 5*sin(menu->frames/80.0 + 20*i);
		}
		else
			glColor4f(1, 1, 1, 0.7);

		draw_text(AL_Left, 20 + s, 20*i, menu->entries[i].name, _fonts.standard);
		
		bind = &(binds[i]);
		
		if(bind->enabled && bind->valcount > 0)
		{
			int len = 0, j;
			for(j = bind->valcount - 1; j+1; --j)
			{
				len += strlen(bind->values[j]);
			
				if(bind->selected == j)
					glColor4f(1,1,0,0.7);
				else
					glColor4f(0.5,0.5,0.5,0.7);
					
				draw_text(AL_Right, SCREEN_W - 200 - (len*20), 20*i, bind->values[j], _fonts.standard);
			}
		}
	}
	
	
	glPopMatrix();
	menu->drawdata[2] += (20*menu->cursor - menu->drawdata[2])/10.0;
	
	fade_out(menu->fade);
	
	glColor4f(1,1,1,1);	
}

void options_menu_input(MenuData *menu) {
	SDL_Event event;
	
	// REMOVE after akari/screenshot is merged
	global_input();
	
	while(SDL_PollEvent(&event)) {
		int sym = event.key.keysym.sym;
		
		// UNCOMMENT after akari/screenshot is merged
		//global_processevent(&event);
		if(event.type == SDL_KEYDOWN) {
			if(sym == tconfig.intval[KEY_DOWN]) {
				menu->drawdata[3] = 10;
				while(!menu->entries[++menu->cursor].action);
				
				if(menu->cursor >= menu->ecount)
					menu->cursor = 0;
				
			} else if(sym == tconfig.intval[KEY_UP]) {
				menu->drawdata[3] = 10;
				while(!menu->entries[--menu->cursor].action);
				
				if(menu->cursor < 0)
					menu->cursor = menu->ecount - 1;
				
			} else if((sym == tconfig.intval[KEY_SHOT] || sym == SDLK_RETURN) && menu->entries[menu->cursor].action) {
				menu->selected = menu->cursor;
				
				OptionBinding *binds = (OptionBinding*)menu->context;
				OptionBinding *bind = &(binds[menu->selected]);
				
				if(bind->enabled)
					binding_setnext(bind);
				else
					menu->quit = 1;
			} else if(sym == tconfig.intval[KEY_LEFT]) {
				menu->selected = menu->cursor;
				OptionBinding *binds = (OptionBinding*)menu->context;
				OptionBinding *bind = &(binds[menu->selected]);
				
				if(bind->enabled)
					binding_setprev(bind);
			} else if(sym == tconfig.intval[KEY_RIGHT]) {
				menu->selected = menu->cursor;
				OptionBinding *binds = (OptionBinding*)menu->context;
				OptionBinding *bind = &(binds[menu->selected]);
				
				if(bind->enabled)
					binding_setnext(bind);
			} else if(sym == SDLK_ESCAPE) {
				menu->quit = 2;
			}
			
			menu->cursor = (menu->cursor % menu->ecount) + menu->ecount*(menu->cursor < 0);
		} else if(event.type == SDL_QUIT) {
			exit(1);
		}
	}
}

int options_menu_loop(MenuData *menu) {
	return menu_loop(menu, options_menu_input, draw_options_menu);
}

