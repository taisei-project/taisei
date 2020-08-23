/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "ending.h"
#include "global.h"
#include "video.h"
#include "progress.h"
#include "dynarray.h"

typedef struct EndingEntry {
	const char *msg;
	Sprite *sprite;
	int time;
} EndingEntry;

typedef struct Ending {
	DYNAMIC_ARRAY(EndingEntry) entries;
	int duration;
	int pos;
	CallChain cc;
} Ending;

static void track_ending(int ending) {
	assert(ending >= 0 && ending < NUM_ENDINGS);
	progress.achieved_endings[ending]++;
}

static void add_ending_entry(Ending *e, int dur, const char *msg, const char *sprite) {
	EndingEntry *entry = dynarray_append(&e->entries);
	entry->time = 0;

	if(e->entries.num_elements > 1) {
		entry->time = 1<<23; // nobody will ever! find out
	}

	entry->msg = msg;

	if(sprite) {
		entry->sprite = res_sprite(sprite);
	} else {
		entry->sprite = NULL;
	}
}

/*
 * These ending callbacks are referenced in plrmodes/ code
 */

void bad_ending_marisa(Ending *e) {
	add_ending_entry(e, -1, "[The Scarlet Devil Mansion]", NULL);

	add_ending_entry(e, -1, "A mansion owned by a pompous vampire, housing her friends and lackeys.", NULL);
	add_ending_entry(e, -1, "Marisa was surrounded by stacks of books, hastily flipping through their pages, drinking tea instead of her usual sake.", NULL);

	add_ending_entry(e, -1, "Marisa: “Hey Patchy, do ya got any more books on Artificin’?”", NULL);
	add_ending_entry(e, -1, "Patchouli: “Yes, but they won’t be of any use. That machine is beyond Artificing entirely.’", NULL);
	add_ending_entry(e, -1, "Marisa: “Ugh, yer probably right, as usual.”", NULL);

	add_ending_entry(e, -1, "Marisa closed the book and let it slam onto the floor.", NULL);
	add_ending_entry(e, -1, "Patchouli: “Mind your manners! That’s a very old tome!”", NULL);
	add_ending_entry(e, -1, "Marisa: “It’s so irritatin’! I thought I solved everything but now that Tower’s just sittin’ there, ‘n the whole Mountain’s been quarantined…!”", NULL);
	add_ending_entry(e, -1, "Marisa: “It just ain’t satisfyin’, y’know?”", NULL);

	add_ending_entry(e, -1, "Flandre: “I could destroy it! Make it go BOOM!”", NULL);
	add_ending_entry(e, -1, "Marisa: “Gah! Now what did I say about sneakin’ up on people, kiddo?”", NULL);
	add_ending_entry(e, -1, "Flandre: “Sorry!”", NULL);
	add_ending_entry(e, -1, "Patchouli: “Thank you for the offer, Flandre, but no.”", NULL);
	add_ending_entry(e, -1, "Flandre: “Awww…”", NULL);

	add_ending_entry(e, -1, "After finishing tea with the pair of them, Marisa wandered the stacks, looking for inspiration.", NULL);

	add_ending_entry(e, -1, "Suddenly, she felt as if she'd stepped into a pothole, tripping and falling flat on her face.", NULL);
	add_ending_entry(e, -1, "But, when she looked back at the floor, she didn't see anything except the expertly-organized rows of immaculate bookshelves.", NULL);
	add_ending_entry(e, -1, "Then, she noticed something cold and thin underneath her hand.", NULL);
	add_ending_entry(e, -1, "It was a ‘Smart Device’ of some kind, like those portable computer-phones from the Outside World.", NULL);
	add_ending_entry(e, -1, "But this one was far more advanced than anything she’d ever seen.", NULL);

	add_ending_entry(e, -1, "It was almost as if it was made of liquid metal, like mercury, but able to take solid shapes on command.", NULL);
	add_ending_entry(e, -1, "As she pressed its buttons, it rapidly cycled through various form factors, such as a thin book, or a handheld phone.", NULL);
	add_ending_entry(e, -1, "Indeed, it was more advanced than anything the Outside World was presently capable of, and not in a style even the Lunarians were known for.", NULL);

	add_ending_entry(e, -1, "Once Marisa worked out how to make it display something, the title of a book appeared on its dim screen…", NULL);
	add_ending_entry(e, -1, "‘Practical & Advanced Computational Applications of the Grand Unified Theory’…", NULL);
	add_ending_entry(e, -1, "… ‘by Usami Renko’", NULL);

	add_ending_entry(e, 300, "-BAD END-\nTo unlock the Extra Stage, keep trying to reach the end without using a Continue!", NULL);
	track_ending(ENDING_BAD_MARISA);
}

void bad_ending_youmu(Ending *e) {
	add_ending_entry(e, -1, "[The Tower of Babel]", NULL);
	add_ending_entry(e, -1, "At the foot of the Tower that now imposed itself on Gensōkyō's skyline…", NULL);
	add_ending_entry(e, -1, "…the greatest swordswoman of Gensōkyō and the Netherworld, Konpaku Yōmu, stood over a makeshift grave, praying.", NULL);
	add_ending_entry(e, -1, "No, she hadn’t slain her opponent. It was a little more troubling than that.", NULL);

	add_ending_entry(e, -1, "Yuyuko: “Tell me again… she simply vanished? In mid-air?”", NULL);
	add_ending_entry(e, -1, "Yōmu: “Yes. One moment she was there, and the next, she was not.”", NULL);
	add_ending_entry(e, -1, "Yōmu: “The vampire from before also disappeared, in addition to all the loitering fairies…”", NULL);
	add_ending_entry(e, -1, "Yōmu: “It was as if everything within it ceased to exist entirely, except myself.”", NULL);

	add_ending_entry(e, -1, "The Tower had ceased functioning, returing the land and yōkai around it to some semblance of status quo, despite its vaguely menacing presence.", NULL);
	add_ending_entry(e, -1, "But the instigators were nowhere to be found, even after much searching by multiple parties.", NULL);

	add_ending_entry(e, -1, "Yōmu’s fearless audacity had dissipated as well, replaced with a sense of loss.", NULL);

	add_ending_entry(e, -1, "Yōmu: “Thus, I must assume that I have caused their deaths in some way, despite my best efforts.”", NULL);
	add_ending_entry(e, -1, "Yōmu: “That woman, Elly… we knew each other for such a short time, yet her passion and fury affected me so deeply.”", NULL);

	add_ending_entry(e, -1, "Yuyuko: “Hmmmm. I didn’t sense her passing through the Netherworld, so I doubt she’s truly died.”", NULL);
	add_ending_entry(e, -1, "Yuyuko: “Although this ‘parallel universes’ stuff is all beyond me, hehehe.”", NULL);

	add_ending_entry(e, -1, "Yōmu took a knee in front of Yuyuko, her hand on the hilt of the Hakurōken, her family's treasured sword.", NULL);
	add_ending_entry(e, -1, "Yōmu: “Thank you for entertaining your humble servant’s odd request, Lady Yuyuko.”", NULL);

	add_ending_entry(e, -1, "Yuyuko: “Oh my, think nothing of it! You did great!”", NULL);
	add_ending_entry(e, -1, "Yuyuko: “But since we’re in the neighbourhood… do you think the kappa are running one of their famous festival grill stands?”", NULL);

	add_ending_entry(e, -1, "Yuyuko: “As a celebration for the incident being resolved, perhaps…?”", NULL);
	add_ending_entry(e, -1, "Yuyuko: “We could go on a fun date! And I’m feeling a bit peckish…”", NULL);

	add_ending_entry(e, -1, "The kappa of Yōkai Mountain were throwing a ‘Gaze Upon The Incredible Tower (From A Safe Distance)’ festival, to encourage human and yōkai tourism after everyone had been scared off by the Tower.", NULL);
	add_ending_entry(e, -1, "Although, Yōmu found it difficult to enjoy herself, of course.", NULL);
	add_ending_entry(e, -1, "Somewhere, somehow, she hoped the furiously righteous scythe-bearer was still out there, waiting for their rematch…", NULL);

	add_ending_entry(e, 300, "-BAD END-\nTo unlock the Extra Stage, keep trying to reach the end without using a Continue!", NULL);
	track_ending(ENDING_BAD_YOUMU);
}

void bad_ending_reimu(Ending *e) {
	add_ending_entry(e, -1, "[The Hakurei Shrine]", NULL);
	add_ending_entry(e, -1, "The shrine at the border of fantasy and reality.", NULL);
	add_ending_entry(e, -1, "The Tower had powered down, yet the instigators were nowhere to be found.", NULL);
	add_ending_entry(e, -1, "Reimu returned to her shrine, unsure if she had done anything at all.", NULL);
	add_ending_entry(e, -1, "The Gods and yōkai of the Mountain shuffled about the shrine quietly, with bated breath.", NULL);

	add_ending_entry(e, -1, "Reimu sighed, and made her announcement.", NULL);
	add_ending_entry(e, -1, "Reimu: “Listen up! I stopped the spread of madness, but until further notice, Yōkai Mountain is off limits!”", NULL);

	add_ending_entry(e, -1, "Nobody was pleased with that.", NULL);
	add_ending_entry(e, -1, "The Moriya Gods were visibly shocked.", NULL);
	add_ending_entry(e, -1, "The kappa demanded that they be able to fetch their equipment and tend to their hydroponic cucumber farms.", NULL);
	add_ending_entry(e, -1, "The tengu furiously scribbled down notes, once again as if they’d had the scoop of the century, before also beginning to make their own demands.", NULL);

	add_ending_entry(e, -1, "Once Reimu had managed to placate the crowd, she sat in the back of the Hakurei Shrine, bottle of sake in hand.", NULL);
	add_ending_entry(e, -1, "She didn’t feel like drinking, however. She nursed it without even uncorking it, sighing to herself.", NULL);

	add_ending_entry(e, -1, "Yukari: “Oh my, a little dissatisfied, aren't we?”", NULL);
	add_ending_entry(e, -1, "Reimu was never surprised by Yukari’s sudden appearances anymore, since she could feel Yukari’s presence long before the ‘gap yōkai’ would make herself known.", NULL);

	add_ending_entry(e, -1, "Yukari: “It’s unlike you to rest until everything is business as usual, Reimu.”", NULL);
	add_ending_entry(e, -1, "Yukari: “Depressed?”", NULL);

	add_ending_entry(e, -1, "Reimu: “Yeah. That place sucked.”", NULL);
	add_ending_entry(e, -1, "Yukari: “Perhaps I should’ve gone with you, hmm?”", NULL);
	add_ending_entry(e, -1, "Reimu: “Huh? Why? Do you wanna gap the whole thing out of Gensōkyō?”", NULL);
	add_ending_entry(e, -1, "Yukari: “That might be a bit much, even for me, my dear.”", NULL);

	add_ending_entry(e, -1, "Reimu: “Whatever that ‘madness’ thing was, it was getting to me, too. It made me feel frustrated and… lonely?”", NULL);
	add_ending_entry(e, -1, "Reimu: “But why would I feel lonely? It doesn’t make any sense.”", NULL);
	add_ending_entry(e, -1, "Yukari: “So, what will you do?”", NULL);

	add_ending_entry(e, -1, "Reimu: “Find a way to get rid of it, eventually. I’m sure there’s an answer somewhere out there…”", NULL);

	add_ending_entry(e, -1, "Yukari leaned against the edge of her gap and smiled.", NULL);
	add_ending_entry(e, -1, "Yukari: “Glad to hear it. It's your duty, after all.”", NULL);
	add_ending_entry(e, 300, "-BAD END-\nTo unlock the Extra Stage, keep trying to reach the end without using a Continue!", NULL);
	track_ending(ENDING_BAD_REIMU);
}

void good_ending_marisa(Ending *e) {
	add_ending_entry(e, -1, "[The Moriya Shrine]", NULL);
	add_ending_entry(e, -1, "A workaholic shrine at the top of Yōkai Mountain.", NULL);
	add_ending_entry(e, -1, "Once defeated, the newcomers agreed to power down their Tower of Babel.", NULL);
	add_ending_entry(e, -1, "Elly and Kurumi were made to scrub the decks of the Moriya Shrine, which had become filthy from the local fairies partying and dueling non-stop during the Incident…", NULL);
	add_ending_entry(e, -1, "… and from some rapidly-diminishing demonstrations from a newly-formed “political bloc”, the “Insect Party.”", NULL);

	add_ending_entry(e, -1, "Sanae: “—which had a brand new proprietary math co-processor enabling advanced vector graphics processing capabilities. Combined with other improvements, such as its DVD drive—", NULL);
	add_ending_entry(e, -1, "Elly: “Ah, I see. Facinating. Wow. Incredible.”", NULL);
	add_ending_entry(e, -1, "Kanako: “You two having fun?”", NULL);
	add_ending_entry(e, -1, "Elly: “… y-yes, ma’am.”", NULL);

	add_ending_entry(e, -1, "Kanako: “Seems like it’s taking a bit more time for the madness to wear off for her. How’s all that ‘knowledge’ working out for you, newcomer?”", NULL);
	add_ending_entry(e, -1, "Sanae: “I remembered way more about retro video game consoles than I wanted to! And now *you’ve* gotta deal with it!”", NULL);
	add_ending_entry(e, -1, "Elly tried to hold back tears of boredom.", NULL);

	add_ending_entry(e, -1, "In the end, the newcomers seemed to calm down once the effects of the Tower had worn off, just like everyone else in Gensōkyō.", NULL);

	add_ending_entry(e, -1, "During an early afternoon, Marisa arrived with a picnic basket and a grin on her face.", NULL);

	add_ending_entry(e, -1, "Elly: “Ah, Ms. Kirisame. Hello again.”", NULL);
	add_ending_entry(e, -1, "Marisa: “Yo, Elly.”", NULL);
	add_ending_entry(e, -1, "Elly: “Do you… need something from me?”", NULL);
	add_ending_entry(e, -1, "Marisa: “Heck yeah I do! A drinkin’ partner!”", NULL);

	add_ending_entry(e, -1, "Elly: “Isn’t it a bit early for that?”", NULL);
	add_ending_entry(e, -1, "Marisa: “Oh c’mon, it’s noon somewhere in the infinite multiverse, ain’t it?”", NULL);
	add_ending_entry(e, -1, "Elly laughed, and decided she needed an excuse for a break anyways.", NULL);

	add_ending_entry(e, -1, "And so, they drank, and caught up on alternate worlds, uncanny timelines, and the strangeness of their circumstances.", NULL);

	add_ending_entry(e, -1, "Marisa: “I mean, ‘course ya lost! Don’t y’know about Isaac Newton?”", NULL);
	add_ending_entry(e, -1, "Elly: “The famous European mathematician? I studied his works, yes. But what about him?”", NULL);
	add_ending_entry(e, -1, "Marisa: “He was an Alchemist!”", NULL);
	add_ending_entry(e, -1, "Elly: “He was a… what?!”", NULL);

	add_ending_entry(e, -1, "Marisa: “Yup. Even got a copy of his occult writings back home.”", NULL);
	add_ending_entry(e, -1, "Marisa: “Pretty cool guy, was into a lotta wild stuff.”", NULL);
	add_ending_entry(e, -1, "Elly deflated, philosophically defeated.", NULL);
	add_ending_entry(e, -1, "Elly: “That… *is* kind of funny, actually. I never ran across any of *those* books of his…”", NULL);

	add_ending_entry(e, -1, "Marisa: “Oh cheer up, will ya? Y’all’ll get to chill out here for as long as ya like.”", NULL);
	add_ending_entry(e, -1, "Elly: “But what if this Gensōkyō collapses, too…?”", NULL);
	add_ending_entry(e, -1, "Marisa: “It could! The Hakurei Barrier could fall tomorrow! But what’s the point in worryin’ about all that?”", NULL);
	add_ending_entry(e, -1, "Marisa: “Right now, I say we enjoy what we got!”", NULL);

	add_ending_entry(e, -1, "Elly: “I suppose you’re right.”", NULL);
	add_ending_entry(e, -1, "Elly: “I have to ask, though… how did you remember me?”", NULL);
	add_ending_entry(e, -1, "Marisa: “Oh, that’s easy. Mushrooms!”", NULL);
	add_ending_entry(e, -1, "Elly: “Mush… rooms? As in, mycelium?”", NULL);

	add_ending_entry(e, -1, "Marisa: “Yeah! A while ago, I was cookin’ up some especially powerful ones I found in a cave near the one-armed hermit's house…”", NULL);
	add_ending_entry(e, -1, "Marisa: “And then, BOOM, there was a small explosion!”", NULL);
	add_ending_entry(e, -1, "Marisa: “Before I knew it was seein’ all sorts’a ‘versions’ of myself out there. My lives were flashin’ before my eyes!”", NULL);
	add_ending_entry(e, -1, "Marisa: “Anyway, took me a bit, but I kinda remembered that vampire girl Kurumi, and when I saw ya, and I was like, ‘Whoa, really?! For real?!’”", NULL);

	add_ending_entry(e, -1, "Elly paused, and then laughed at the ridiculousness of it all.", NULL);
	add_ending_entry(e, -1, "Elly: “That’s it, then? But I suppose in a place like Gensōkyō, that's not even that strange…”", NULL);
	add_ending_entry(e, -1, "Elly: “… and you were such a child when I met you. It’s odd to see you all grown up, even if it’s another you entirely.”", NULL);
	add_ending_entry(e, -1, "Marisa: “Yup, humans grow up. I’m completely ordinary, after all.”", NULL);

	add_ending_entry(e, -1, "Many residents of Gensokyo wondered if the Tower would be a blight on the skyline for all eternity.", NULL);
	add_ending_entry(e, -1, "Others questioned if its power could be harnessed to serve the greater good.", NULL);
	add_ending_entry(e, -1, "But for the guardians of Gensōkyō, they would need to be vigilant, lest it suddenly spring to life again…", NULL);
	add_ending_entry(e, 300, "-GOOD ENDING-\nOne Credit Clear Achieved!\nExtra Stage has been unlocked! Do you have what it takes to face the true nature of reality?", NULL);
	track_ending(ENDING_GOOD_MARISA);
}

void good_ending_youmu(Ending *e) {
	add_ending_entry(e, -1, "[The Moriya Shrine]", NULL);
	add_ending_entry(e, -1, "A workaholic shrine at the top of Yōkai Mountain.", NULL);
	add_ending_entry(e, -1, "The Tower of Babel stood silently nearby in the skyline, as if a sleeping giant.", NULL);
	add_ending_entry(e, -1, "Below, at the not-so-humble Moriya Shrine, two blademasters were exchanging the ways of their craft.", NULL);

	add_ending_entry(e, -1, "Elly: “I’m sure you’re aware that the classic scythe such as this was primarily used by farmers, as a makeshift weapon—”", NULL);
	add_ending_entry(e, -1, "Yōmu: “Yes, of course. Many of the most advanced martial arts evolved from the practices of common folk—”", NULL);

	add_ending_entry(e, -1, "After the incident, Elly and Kurumi agreed to surrender, and disabled the Tower's ability to ‘enlighten’.", NULL);
	add_ending_entry(e, -1, "Their dispositions softened considerably, and they apologized profusely for everything they had done.", NULL);
	add_ending_entry(e, -1, "But even as everyone else returned to their usual selves, it seemed that Konpaku Yōmu had retained nearly all of the newfound confidence she had gained during the incident…", NULL);

	add_ending_entry(e, -1, "Elly: “This Gensōkyō is so different compared to my own.”", NULL);
	add_ending_entry(e, -1, "Yōmu: “Oh? What was your Gensōkyō like?”", NULL);
	add_ending_entry(e, -1, "Elly: “Mostly, a lot smaller. It felt more like a road-side food stand. Nothing remarkable about it.”", NULL);
	add_ending_entry(e, -1, "Yōmu: “Hmm, fascinating…”", NULL);

	add_ending_entry(e, -1, "Elly: “Well, I ought to return to my duties at the shrine.”", NULL);
	add_ending_entry(e, -1, "Elly: “I hope the green-haired shrine maiden doesn’t talk my ear off about obsolete entertainment technology again…”", NULL);
	add_ending_entry(e, -1, "Yōmu: “I commend you for complying with your penance, despite its… cruel hardships.”", NULL);
	add_ending_entry(e, -1, "The pair of them laughed.", NULL);

	add_ending_entry(e, -1, "Elly: “I know I said this before, but I still feel like a fool.”", NULL);
	add_ending_entry(e, -1, "Elly: “I was miserable about my home, but that was going too far…”", NULL);
	add_ending_entry(e, -1, "Elly: “It seems like the one most overcome by madness was myself.”", NULL);
	add_ending_entry(e, -1, "Elly: “I know everyone here accepted my apology, but I’m still truly sorry for what I did.”", NULL);

	add_ending_entry(e, -1, "Yōmu put a hand on Elly’s shoulder and smiled.", NULL);
	add_ending_entry(e, -1, "Yōmu: “Your motivations were righteous, even if your methods were not.”", NULL);
	add_ending_entry(e, -1, "Yōmu: “Now that you’ve recognized that, you are welcome here, like anyone else.”", NULL);
	add_ending_entry(e, -1, "Yōmu: “Tomorrow, provided Lady Yuyuko has no business for me, I would like to show you what the Netherworld and Gensōkyō have to offer!", NULL);
	add_ending_entry(e, -1, "Yōmu: “Perhaps we could even visit a popular ramen shop nearby. I’m feeling generous, so it shall be my treat!”", NULL);

	add_ending_entry(e, -1, "Elly hesitated, but then happily agreed.", NULL);
	add_ending_entry(e, -1, "As they spent many evenings dueling, practicing, and drinking merrily, many commented on how they had never seen Yōmu so thrilled.", NULL);
	add_ending_entry(e, -1, "Truly, the pair of them had become practically inseparable.", NULL);

	add_ending_entry(e, -1, "Despite the incident being resolved, nobody had figured out how to remove the Tower, and most were too anxious to even think about it.", NULL);
	add_ending_entry(e, -1, "And with its lingering, menacing visage blighting the skyline, many wondered if the Tower was truly inert and harmless.", NULL);
	add_ending_entry(e, -1, "Meanwhile, in the Netherworld, a certain gap yōkai and her ghostly companion discussed details of an ancient future’s past…", NULL);

	add_ending_entry(e, 300, "-GOOD ENDING-\nOne Credit Clear Achieved!\nExtra Stage has been unlocked! Do you have what it takes to face the true nature of reality?", NULL);
	track_ending(ENDING_GOOD_YOUMU);
}

void good_ending_reimu(Ending *e) {
	add_ending_entry(e, -1, "[The Moriya Shrine]", NULL);
	add_ending_entry(e, -1, "A rival shrine at the top of Yōkai Mountain.", NULL);

	add_ending_entry(e, -1, "Reimu: “Listen, Sanae, I don’t know what a ‘speedrun’ is, and I really don’t care either!”", NULL);
	add_ending_entry(e, -1, "Sanae: “You’re such a stick in the mud, Reimu! Listen, you like resolving incidents as quickly as possible, right?”", NULL);
	add_ending_entry(e, -1, "Sanae: “It’s like that, except with video games!”", NULL);
	add_ending_entry(e, -1, "Reimu: “I don’t know anything about ‘video games’ either!”", NULL);

	add_ending_entry(e, -1, "The Tower had been shut down, ending the crisis of madness sweeping the land.", NULL);
	add_ending_entry(e, -1, "Nearly everyone had returned to normal, although a few odd souls lamented their loss of ‘knowledge’.", NULL);
	add_ending_entry(e, -1, "Kurumi and Elly had been conscripted into sweeping the paths of the Moriya Shrine as penance for their actions.", NULL);
	add_ending_entry(e, -1, "With the occasional odd request from the Moriya Shrine Gods, of course…", NULL);

	add_ending_entry(e, -1, "Kanako: “I can’t believe it! Is it really that simple?!”", NULL);
	add_ending_entry(e, -1, "Elly: “Y-yes, it’s just a matter of changing that coefficient there, and shaping the magnetic containment field using—”", NULL);
	add_ending_entry(e, -1, "Kanako: “Of course! I was such a fool! I can’t believe I ever bothered attempting to harness the Yatagarasu!”", NULL);
	add_ending_entry(e, -1, "Kanako: “At last, I have the final key to modernizing Gensōkyō, and preventing the energy crisis from reaching us! Fusion power is now attainable!”", NULL);

	add_ending_entry(e, -1, "Elly: “A-are you sure you’re well, ma’am?”", NULL);
	add_ending_entry(e, -1, "Suwako: “Heh heh, don’t worry. This is as normal as it gets for Kanako.”", NULL);
	add_ending_entry(e, -1, "Elly: “‘Normal’? Really?”", NULL);
	add_ending_entry(e, -1, "Suwako: “Don’t you think she’s cute when she’s like this?”", NULL);

	add_ending_entry(e, -1, "Reimu was spending more time on Yōkai Mountain, claiming she was “keeping watch.”", NULL);
	add_ending_entry(e, -1, "Reimu: “Hey, Elly.”", NULL);
	add_ending_entry(e, -1, "Elly: “Ah, yes? Do you need something from me?”", NULL);

	add_ending_entry(e, -1, "Reimu: “I’m surprised you’re still not pissed off at everyone. Not even a little bit.”", NULL);
	add_ending_entry(e, -1, "Reimu: “You were shouting like a dictator up there.”", NULL);
	add_ending_entry(e, -1, "Elly: “It seems that the Tower's influence got to me, in the end. I thought I was stronger than that, but…”", NULL);
	add_ending_entry(e, -1, "Elly: “Of course, I’m still sad about my own Gensōkyō, but to go that far…”", NULL);

	add_ending_entry(e, -1, "Reimu: “What a weird machine. I wonder what the kappa would think of it.”", NULL);
	add_ending_entry(e, -1, "Suwako: “The kappa won’t touch the thing, and even Kanako has talked herself out of it.”", NULL);
	add_ending_entry(e, -1, "Reimu: “Still odd how those two were forgotten from Gensōkyō, though…”", NULL);
	add_ending_entry(e, -1, "Reimu: “Ah, whatever. As long as it stays quiet.”", NULL);

	add_ending_entry(e, -1, "With the maddening feelings gone, everyone seemed to relax more than usual, like a burden had been lifted off of them.", NULL);
	add_ending_entry(e, -1, "But for some reason, Reimu in particular seemed to have a difficult time settling down.", NULL);

	add_ending_entry(e, -1, "Even as some grew to accept the Tower of Babel looming in the skyline, there were frequent mutterings over potentially getting rid of it entirely.", NULL);
	add_ending_entry(e, -1, "Although, nobody had any clue how, except, perhaps, a certain ‘gap yōkai’…", NULL);

	add_ending_entry(e, 300, "-GOOD ENDING-\nOne Credit Clear Achieved!\nExtra Stage has been unlocked! Do you have what it takes to face the true nature of reality?", NULL);
	track_ending(ENDING_GOOD_REIMU);
}

static void init_ending(Ending *e) {
	dynarray_ensure_capacity(&e->entries, 32);

	if(global.plr.stats.total.continues_used) {
		global.plr.mode->character->ending.bad(e);
	} else {
		global.plr.mode->character->ending.good(e);
		add_ending_entry(e, 400, "-UNDER CONSTRUCTION-\nExtra Stage coming soon! Please wait warmly for it!", NULL);
	}

	add_ending_entry(e, 400, "", NULL);  // this is important   <-- TODO explain *why* this is important?
	e->duration = 1<<23;
}

static void ending_draw(Ending *e) {
	float s, d;

	EndingEntry *e_this, *e_next;
	e_this = dynarray_get_ptr(&e->entries, e->pos);
	e_next = dynarray_get_ptr(&e->entries, e->pos + 1);

	int t1 = global.frames - e_this->time;
	int t2 = e_next->time - global.frames;

	d = 1.0/ENDING_FADE_TIME;

	if(t1 < ENDING_FADE_TIME)
		s = clamp(d*t1, 0.0, 1.0);
	else
		s = clamp(d*t2, 0.0, 1.0);

	r_clear(CLEAR_ALL, RGBA(0, 0, 0, 1), 1);

	r_color4(s, s, s, s);

	if(e_this->sprite) {
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = e_this->sprite,
			.pos = { SCREEN_W/2, SCREEN_H/2 },
			.shader = "sprite_default",
			.color = r_color_current(),
		});
	}

	r_shader("text_default");
	text_draw_wrapped(e_this->msg, SCREEN_W * 0.85, &(TextParams) {
		.pos = { SCREEN_W/2, VIEWPORT_H*4/5 },
		.align = ALIGN_CENTER,
	});
	r_shader_standard();

	r_color4(1,1,1,1);
}

void ending_preload(void) {
	preload_resource(RES_BGM, "ending", RESF_OPTIONAL);
}

static void ending_advance(Ending *e) {
	EndingEntry *e_this, *e_next;
	e_this = dynarray_get_ptr(&e->entries, e->pos);
	e_next = dynarray_get_ptr(&e->entries, e->pos + 1);

	if(
		e->pos < e->entries.num_elements - 1 &&
		e_this->time + ENDING_FADE_TIME < global.frames &&
		global.frames < e_next->time - ENDING_FADE_TIME
	) {
		// TODO find out wtf that last part is for
		e_next->time = global.frames + (e->pos != e->entries.num_elements - 2) * ENDING_FADE_TIME;
	}
}

static bool ending_input_handler(SDL_Event *event, void *arg) {
	Ending *e = arg;

	TaiseiEvent type = TAISEI_EVENT(event->type);
	int32_t code = event->user.code;

	switch(type) {
		case TE_GAME_KEY_DOWN:
			if(code == KEY_SHOT) {
				ending_advance(e);
			}
			break;
		default:
			break;
	}
	return false;
}

static LogicFrameAction ending_logic_frame(void *arg) {
	Ending *e = arg;

	update_transition();
	events_poll((EventHandler[]){
		{ .proc = ending_input_handler, .arg=e },
		{ NULL }
	}, EFLAG_GAME);

	global.frames++;

	if(e->pos < e->entries.num_elements - 1 && global.frames >= dynarray_get(&e->entries, e->pos + 1).time) {
		e->pos++;
		if(e->pos == e->entries.num_elements - 1) {
			audio_bgm_stop((FPS * ENDING_FADE_OUT) / 4000.0);
			set_transition(TransFadeWhite, ENDING_FADE_OUT, ENDING_FADE_OUT);
			e->duration = global.frames+ENDING_FADE_OUT;
		}

	}

	if(global.frames >= e->duration) {
		return LFRAME_STOP;
	}

	if(gamekeypressed(KEY_SKIP)) {
		ending_advance(e);
		return LFRAME_SKIP;
	}

	return LFRAME_WAIT;
}

static RenderFrameAction ending_render_frame(void *arg) {
	Ending *e = arg;

	if(e->pos < e->entries.num_elements - 1) {
		ending_draw(e);
	}

	draw_transition();
	return RFRAME_SWAP;
}

static void ending_loop_end(void *ctx) {
	Ending *e = ctx;
	CallChain cc = e->cc;
	dynarray_free_data(&e->entries);
	free(e);
	progress_unlock_bgm("ending");
	run_call_chain(&cc, NULL);
}

void ending_enter(CallChain next) {
	ending_preload();

	Ending *e = calloc(1, sizeof(Ending));
	init_ending(e);
	e->cc = next;

	global.frames = 0;
	set_ortho(SCREEN_W, SCREEN_H);
	audio_bgm_play(res_bgm("ending"), true, 0, 0);

	eventloop_enter(e, ending_logic_frame, ending_render_frame, ending_loop_end, FPS);
}
