/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "ending.h"
#include "global.h"
#include "video.h"
#include "progress.h"

static void track_ending(int ending) {
	assert(ending >= 0 && ending < NUM_ENDINGS);
	progress.achieved_endings[ending]++;
}

void add_ending_entry(Ending *e, int dur, char *msg, char *tex) {
	EndingEntry *entry;
	e->entries = realloc(e->entries, (++e->count)*sizeof(EndingEntry));
	entry = &e->entries[e->count-1];

	if(dur == -1)
		dur=max(300,2.5*strlen(msg)); // i know unicode, but this just needs to be roughly right
	entry->time = e->duration;
	e->duration += dur;
	entry->msg = NULL;
	stralloc(&entry->msg, msg);

	if(tex)
		entry->tex = get_tex(tex);
	else
		entry->tex = NULL;
}

/*
 * These ending callbacks are referenced in plrmodes/ code
 */

void bad_ending_marisa(Ending *e) {
	add_ending_entry(e, -1, "“Now, let your mind be expanded! Do you have the potential to understand the code behind reality?”", NULL);
	add_ending_entry(e, -1, "Unfortunately for Marisa, despite her great efforts at comprehension using the greatest stretches of her magician’s genius, her mind was too small yet. Overwhelmed by the ultimate truth, she cried out as as her broom staggered beneath her weight and plummeted like melting wax wings before the heat of the sun. Before she could do anything to correct its trajectory, her vision swam with light and the mysterious world created by logic dissipated into a seamless haze, before melting into darkness.", NULL);
	add_ending_entry(e, -1, "When she awoke, she found herself mysteriously slumped against the doorway of her house in the Forest of Magic, seemingly as if she had never begun her adventure at all. Clutching at her pounding head, she gathered up her fallen broom and disheveled hat, and in doing so realized that Aya had left an innocuous-looking newspaper jammed under the doorway.", NULL);
	add_ending_entry(e, -1, "“‘Culprit Wriggle Defeated by Marisa in Grand Battle at the End of the Tunnel!’ …What?” Marisa furrowed her brow in confusion as she read the paper out loud. “That squirt was just bluffin’! I fought someone else after her; I’m sure about this! …But who was it?” She racked her brain futilely, but neither she nor the silent woods around her offered an answer.", NULL);
	add_ending_entry(e, -1, "“I never forget anyone that I’ve battled so maybe the news is right. Wriggle must have done everything after all, that crafty lil’ bug!” Sighing, she folded up the paper, and went on with her day, which was filled with praise for Wriggle’s defeat from various Gensōkyō residents, leaving the magician with mixed feelings.", NULL);
	add_ending_entry(e, -1, "As such, despite feeling drained at the end of the day, Marisa didn’t get much sleep that night as she continued to ruminate over this unreasonable conclusion. Everyone else had accepted the firefly as the mastermind of the incident so why did it continue to feel like such a falsehood? If only she could redo the day of the incident; she was certain that if she did she would find someone far more logical at the end of the tunnel.", NULL);
	add_ending_entry(e, 300, "-BAD END-\nTry a clear without continues next time!", NULL);
	track_ending(ENDING_BAD_1);
}

void bad_ending_youmu(Ending *e) {
	add_ending_entry(e, -1, "“Now, let your mind be expanded! Do you have the potential to understand the code behind reality?”", NULL);
	add_ending_entry(e, -1, "Although Yōmu reached into the deepest recesses of her samurai mental concentration, she was not a fully budded warrior, and so found this spring of knowledge far beyond her grasp. She instead bloomed with shame as she felt her mind be consumed by the light, no matter how much she struggled against it, and soon even the horizon swirled around her incomprehensibly.", NULL);
	add_ending_entry(e, -1, "Her spirit half was not spared either, finding itself stretched and distorted into strange, meaningless shapes, and together both halves of Yōmu sank down into the foggy abyss of forgetfulness as the world dissipated before them like a fragile dawn mist.", NULL);
	add_ending_entry(e, -1, "“Yōmu… Yōmu, wake up.”", NULL);
	add_ending_entry(e, -1, "Bolting upright in shock, Yōmu found herself in her bed back in Hakugyokurō. It was early morning, just as when she had left to solve the incident, but immediately something felt off, and the half-ghost gardener found herself feeling surprisingly flustered and discontented.", NULL);

	add_ending_entry(e, -1, "“I’m sorry, Lady Yuyuko! Did I oversleep? I feel like something important is supposed to happen today!”", NULL);
	add_ending_entry(e, -1, "“Aha! Just as I expected, you must be tired from solving that incident yesterday! I suppose even bugs can be properly scary when you face them at the wrong times!” Yuyuko said with a small laugh, beaming with pride at her beloved servant.", NULL);
	add_ending_entry(e, -1, "“Wait, pardon? Did you just say that an insect caused the entire incident with the Netherworld boundary?!” Yōmu questioned confoundedly, nursing a strangely powerful headache as she stared at her mistress with her mouth agape.", NULL);
	add_ending_entry(e, -1, "“Why, yes. Surely you must remember defeating Wriggle,” Yuyuko insisted, raising an eyebrow at Yōmu as she stumbled over her fractured memories.", NULL);
	add_ending_entry(e, -1, "“Of course I do! I valiantly battled her and her insect infestation at the end of the Tunnel of Light. But I encountered someone else afterwards in a grand mansion! …At least I think I did?”", NULL);
	add_ending_entry(e, -1, "“Don’t be silly! You came back right after you fought the insect and told me that the tunnel closed. How could you have possibly seen anyone else in a place that doesn’t exist?” Yuyuko said, laughing a bit as she was amused by the contradictory case her assistant was presenting. “You defeated the powerful Nightbug and stopped the nefarious boundary break. Stop overthinking things and come make me breakfast!”", NULL);
	add_ending_entry(e, -1, "“Understood. I’m sorry for muddling things, mistress! I’ll go ahead and cook something right away,” Yōmu finally agreed, hoping to placate her lady’s impatient and insatiable appetite that she was foolishly drawing out.", NULL);
	add_ending_entry(e, -1, "But despite Yuyuko’s complete acceptance of the incident’s resolution, Yōmu still couldn’t shake the instinct that something wasn’t correct and that she had failed critically in some manner. If only she could visit the events over again… maybe then she could cut to the heart of the matter as she had promised.", NULL);
	add_ending_entry(e, -1, "-BAD END-\nTry a clear without continues next time!", NULL);
	track_ending(ENDING_BAD_2);
}

void bad_ending_reimu(Ending *e) {
	add_ending_entry(e, -1, "“Now, let your mind be expanded! Do you have the potential to understand the code behind reality?”", NULL);
	add_ending_entry(e, -1, "Reimu may have been the mouthpiece of the gods, but this truth was far beyond the words they would push into her brain. Numbers overlapped endlessly and seamlessly over each other, and she quickly found herself in an inescapable mental haze. Without realizing it, she had stopped floating, and the Yin-Yang Orbs dropped uselessly alongside her, having lost all of their holy powers.", NULL);
	add_ending_entry(e, -1, "Tired of what she perceived as nonsense, she willed the numbers to end, and they did, taking any directives of the Hakurei god along with them to create an unbearable silence that drifted darkly into unconsciousness.", NULL);
	add_ending_entry(e, -1, "“Auughh!” \nBolting upwards in her futon, Reimu yelled out both in inexplicable frustration and pain as a sudden migraine woke her up on an otherwise peaceful morning. Grumbling, she glanced all around her room instinctually, as if she had seen something wrong with it before.", NULL);
	add_ending_entry(e, -1, "But it looked just the same as it normally did, even through the angry throbbing in her skull. Knowing that further sleep was impossible, however, she picked herself up and started to get ready for the rest of the day. Of course, she didn’t get too far before being interrupted by an overly familiar yōkai sage.", NULL);
	add_ending_entry(e, -1, "“What do you want?” Reimu asked grouchily, rubbing her forehead as she stared at a strangely concerned Yukari.", NULL);
	add_ending_entry(e, -1, "“Did you really defeat Wriggle as the main culprit of the incident yesterday?”", NULL);
	add_ending_entry(e, -1, "“Huh? I don’t even remember who I fought. Does it even matter?”", NULL);
	add_ending_entry(e, -1, "“Yes, it does! Because Wriggle shouldn’t have that sort of power. That means… she must be more dangerous than she has been letting on! Never mind, fufu!” ", NULL);
	add_ending_entry(e, -1, "Watching Yukari change her tone mid-sentence, Reimu felt disconcerted. The gap hag must have been onto something, but just as soon as she had realized it, that thought had been taken away by a mysterious force.", NULL);
	add_ending_entry(e, -1, "Clearly this peace she had achieved by defeating Wriggle was just a farce, but she had no idea who was really behind it. What she needed now was another chance in another time-- to find the real logical perpetrator of the incident, she would have to go back and do it all over again.", NULL);
	add_ending_entry(e, -1, "-BAD END-\nTry a clear without continues next time!", NULL);
	track_ending(ENDING_BAD_3);
}

void good_ending_marisa(Ending *e) {
	add_ending_entry(e, -1, "“Now, let your mind be expanded! Do you have the potential to understand the code behind reality?”", NULL);
	add_ending_entry(e, -1, "“Ha! Who do ya take me for? Of course!”", NULL);
	add_ending_entry(e, -1, "Marisa blasted through the mental bombardment with incredible confidence, striking Elly with the full force of a blazing star. The artificial world shuddered from the force of the impact, and then Tower of Babel itself explosively blew apart in a catastrophic fireball, forcing the two combatants to land on the grassy plain below it, where Elly herself sat stunned into silence for several moments.", NULL);
	add_ending_entry(e, -1, "“Imagine if I had actually unraveled Gensōkyō’s truth…!” she finally said, shaking her head. “It may be for the best. Even Gensōkyō has its own form of common sense, and once that is gone, there is no telling what could be lost.”", NULL);
	add_ending_entry(e, -1, "“Right,” Marisa agreed, strangely thoughtful compared to her brash front, “but there is really somethin’ more to this place, isn’t there? I can’t stop now! I’l be the one to find the truth for ya!”", NULL);
	add_ending_entry(e, -1, "“But what about me? My research, and everything I’ve worked for is…” Elly interjected, but Marisa stopped her from further lamentation by patting her back cheerfully.", NULL);
	add_ending_entry(e, -1, "“Did’ja forget? Gensōkyō takes pretty much anyone. I’m sure there are a few of us that’ll be pretty excited to see ya and Kurumi again. And who knows; the Human Village could always use a cool new professor in town, right? There’s no reason to throw away everything you’ve learned!”", NULL);
	add_ending_entry(e, -1, "This time, Elly was able to smile back, and after standing and picking up her scythe, she watched as Marisa mounted her broom and soared off freely into the expanding distance. After witnessing the burning passion of Marisa’s fire, she was confident that the witch was perfectly suited to pick up in her stead and find the answer.", NULL);
	add_ending_entry(e, -1, "The only question now is what secret Marisa would, in fact, see at the end of the world.", NULL);
	add_ending_entry(e, -1, "-GOOD END-\nExtra Stage has opened up for you! Do you have what it takes to discover the hidden truth behind everything?", NULL);

	track_ending(ENDING_GOOD_1);
}

void good_ending_youmu(Ending *e) {
	add_ending_entry(e, -1, "“Now, let your mind be expanded! Do you have the potential to understand the code behind reality?”", NULL);
	add_ending_entry(e, -1, "“I must, in order to save Gensōkyō and the Netherworld! Nothing stands in my way that cannot be sliced through!”", NULL);
	add_ending_entry(e, -1, "With steeled persistence, Yōmu charged through the mental bombardment, and drawing her mystical Rōkanken, she clashed her blade against Elly’s shimmering scythe, the two meeting in a furious clamor that echoed through the empty world around them.", NULL);
	add_ending_entry(e, -1, "Trembling beneath the power of the yōkai-tempered sword, the scythe finally folded and Yōmu slashed through the Tower of Babel, both halves peeling away and then crumbling to dust. In the roar that followed, both Yōmu and Elly found themselves on the plains below the tower, the former gatekeeper now staring wistfully at her scythe’s severed blade as she held up its empty handle.", NULL);
	add_ending_entry(e, -1, "“You’ve destroyed everything. My research, my weapon, and now even my truth,” Elly cried out, wiping tears with her free hand.", NULL);
	add_ending_entry(e, -1, "“No, not everything. You may not realize it, but I didn’t cut apart your future. There is no one in Gensōkyō that wouldn’t welcome you, so why not come there? You would find expert blacksmiths to make you an even more beautiful scythe, as well,” Yōmu said calmly, picking the curved pieces of Elly’s weapon carefully and giving them back to her.", NULL);
	add_ending_entry(e, -1, "“But what about the truth? In the end, I was unable to complete my findings of it. Will it lay unknown to all forever?” Elly asked, to which Yōmu raised her sword with a cool glint in her eye.", NULL);
	add_ending_entry(e, -1, "“Of course not. My sacred duty is to cut through whatever necessary in order to discover the real meanings behind incidents, for the sake of both the living and the dead. We’re not done here, and I’ll be the one to find the real truth behind everything for you!”", NULL);
	add_ending_entry(e, -1, "Elly was unable to hold back her sense of awe as she watched Yōmu sheathe her blade with perfect composure and then leap back into the air with astounding speed. After witnessing the cold determination of Yōmu’s nerve, she was certain that the half-ghost samurai was perfectly suited to pick up in her stead and find the answer.", NULL);
	add_ending_entry(e, -1, "The only question now is what secret Yōmu would, in fact, see at the end of the world.", NULL);
	add_ending_entry(e, -1, "-GOOD END-\nExtra Stage has opened up for you! Do you have what it takes to discover the hidden truth behind everything?", NULL);
	track_ending(ENDING_GOOD_2);
}

void good_ending_reimu(Ending *e) {
	add_ending_entry(e, -1, "“Now, let your mind be expanded! Do you have the potential to understand the code behind reality?”", NULL);
	add_ending_entry(e, -1, "“What a pointless question! Be defeated already!”", NULL);
	add_ending_entry(e, -1, "Reimu, with her straightforward clarity, was able to hold fast and ignore the barrage focused upon her mind by sealing herself off via her fantasy nature. Raising her purification rod as a command, her Yin-Ying Orbs soared into the false sky and coalesced into one, a glowing orb that quickly grew to humongous proportions.", NULL);
	add_ending_entry(e, -1, "She flicked her rod downwards, and Elly found herself facing this massive spirit orb unprepared. It plowed into her, and then also into her tower, smashing all in its path with a godly roar. When the dust and rubble settled, Reimu landed on the plains below, in front of the weakened Elly.", NULL);
	add_ending_entry(e, -1, "“Are you satisfied? You destroyed my research, my life, and my home. And for what? An illogical world fit for neither yōkai nor humans!” Elly shouted, trying to but failing to crawl towards her fallen scythe.", NULL);
	add_ending_entry(e, -1, "“That’s not true. You’re upset because you blame Gensōkyō for rejecting you when the reality is that you simply got lost, didn’t you? Gensōkyō welcomes everyone, and it will still welcome you,” Reimu replied, kicking aside Elly’s scythe before reaching out to pull her up. Elly hesitantly took it, her knees still shaking from Reimu’s spiritual blow.", NULL);
	add_ending_entry(e, -1, "“I may be able to return to Gensōkyō as a yōkai, but what will happen to my work? I was so close to the truth and you blew it all away. There's no way I could be satisfied without knowing!”", NULL);
	add_ending_entry(e, -1, "Reimu mulled over this for a moment before swinging her rod over her back, looking suddenly determined.", NULL);
	add_ending_entry(e, -1, "“This truth of yours almost pulled Gensōkyō apart. For that I’ll just have to figure out the rest in your stead and then deal with it in any way that I’ll need to. Seems like I can’t go home yet…” Reimu declared, breaking it off with a sigh at the end.", NULL);
	add_ending_entry(e, -1, "“Oh well, I’ll just make this as quick as I can. Feel free to invite yourself over to my shrine if you want. Everyone does that even if I say no, so I can’t really stop you.”", NULL);
	add_ending_entry(e, -1, "Elly blinked for a few seconds in shock, and then started to laugh. Just like in the old days, Reimu still treated everyone with perfect neutrality. If anyone was best suited to discovering something without biases, it was Reimu, the Shrine Maiden of Paradise.", NULL);
	add_ending_entry(e, -1, "Reimu herself had already started floating away towards the artificial horizon, and Elly watched, now completely at ease. After all, if Reimu was on the case, it would always work out right in the end. She was the unavoidable constant in a sea of variables, and using her shrine maiden intuition as her compass, there was no way that she could fail to trailblaze the way to the truth.", NULL);
	add_ending_entry(e, -1, "The only question now is what secret Reimu would, in fact, see at the end of the world.", NULL);
	add_ending_entry(e, -1, "-GOOD END-\nExtra Stage has opened up for you! Do you have what it takes to discover the hidden truth behind everything?", NULL);
	track_ending(ENDING_GOOD_3);
}

void create_ending(Ending *e) {
	memset(e, 0, sizeof(Ending));

	if(global.plr.continues_used) {
		global.plr.mode->character->ending.bad(e);
	} else {
		global.plr.mode->character->ending.good(e);
		add_ending_entry(e, 400, "Sorry, extra stage isn’t done yet. ^^", NULL);
	}

	if(global.diff == D_Lunatic)
		add_ending_entry(e, 400, "Lunatic? Didn’t know this was even possible. Cheater.", NULL);

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

	draw_text_auto_wrapped(AL_Center, SCREEN_W/2, VIEWPORT_H*4/5, e->entries[e->pos].msg, SCREEN_W * 0.85, _fonts.standard);
	glColor4f(1,1,1,1);

	draw_and_update_transition();
}

void ending_preload(void) {
	preload_resource(RES_BGM, "ending", RESF_OPTIONAL);
}

static bool ending_frame(void *arg) {
	Ending *e = arg;

	events_poll(NULL, 0);

	if(e->pos >= e->count - 1) {
		return false;
	}

	ending_draw(e);
	global.frames++;
	SDL_GL_SwapWindow(video.window);

	if(global.frames >= e->entries[e->pos+1].time) {
		e->pos++;
	}

	if(global.frames == e->entries[e->count-1].time-ENDING_FADE_OUT) {
		fade_bgm((FPS * ENDING_FADE_OUT) / 4000.0);
		set_transition(TransFadeWhite, ENDING_FADE_OUT, ENDING_FADE_OUT);
	}

	return true;
}

void ending_loop(void) {
	Ending e;
	ending_preload();
	create_ending(&e);
	global.frames = 0;
	set_ortho();
	start_bgm("ending");
	loop_at_fps(ending_frame, NULL, &e, FPS);
	free_ending(&e);
}
