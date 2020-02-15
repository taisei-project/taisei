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

typedef struct EndingEntry {
	char *msg;
	Sprite *sprite;
	int time;
} EndingEntry;

typedef struct Ending {
	EndingEntry *entries;
	int count;
	int duration;
	int pos;
	CallChain cc;
} Ending;

static void track_ending(int ending) {
	assert(ending >= 0 && ending < NUM_ENDINGS);
	progress.achieved_endings[ending]++;
}

static void add_ending_entry(Ending *e, int dur, const char *msg, const char *sprite) {
	EndingEntry *entry;
	e->entries = realloc(e->entries, (++e->count)*sizeof(EndingEntry));
	entry = &e->entries[e->count-1];

	entry->time = 0;

	if(e->count > 1) {
		entry->time = 1<<23; // nobody will ever! find out
	}

	entry->msg = NULL;
	stralloc(&entry->msg, msg);

	if(sprite) {
		entry->sprite = get_sprite(sprite);
	} else {
		entry->sprite = NULL;
	}
}

/*
 * These ending callbacks are referenced in plrmodes/ code
 */

void bad_ending_marisa(Ending *e) {
	add_ending_entry(e, -1, "[The Scarlet Devil Mansion]", NULL);

	add_ending_entry(e, -1, "A mansion owned by a pompous vampire, housing her friends and lackies.", NULL);
	add_ending_entry(e, -1, "Marisa was surrounded by stacks of books, hastily flipping through their pages, drinking tea instead of her usual sake.", NULL);

	add_ending_entry(e, -1, "Marisa: ”Hey Patchy, do ya got any more books on Artificin’?”", NULL);
	add_ending_entry(e, -1, "Patchouli: ”Yes, but they won’t be of any use. That machine is beyond Artificing entirely.’", NULL);
	add_ending_entry(e, -1, "Marisa: ”Ugh, yer probably right, as usual.”", NULL);

	add_ending_entry(e, -1, "Marisa closed the book and let it slam onto the floor.", NULL);
	add_ending_entry(e, -1, "Patchouli: ”Mind your manners! That’s a very old tome!”", NULL);
	add_ending_entry(e, -1, "Marisa: ”It’s so irritatin’! I thought I solved everything but now that tower’s just sittin’ there, ’n the whole Mountain’s been quarantined…”", NULL);

	add_ending_entry(e, -1, "Marisa: ”It just ain’t satisfyin’, y’know?”", NULL);
	add_ending_entry(e, -1, "Flandre: ”I could destroy it! Make it go BOOM!”", NULL);
	add_ending_entry(e, -1, "Marisa: ”Gah! Now what did I say about sneakin’ up on people, kiddo?”", NULL);
	add_ending_entry(e, -1, "Flandre: ”Sorry!”", NULL);
	add_ending_entry(e, -1, "Patchouli: ”Thank you for the offer, Flandre, but no.”", NULL);
	add_ending_entry(e, -1, "Flandre: ”Awww…”", NULL);

	add_ending_entry(e, -1, "After finishing tea with the pair of them, Marisa wandered the stacks, looking for inspiration.", NULL);
	add_ending_entry(e, -1, "Somehow, she tripped on her own feet and landed face-first against a particularly hard and sharp-edged book.", NULL);
	add_ending_entry(e, -1, "Upon picking it up, she noticed it was actually an ’Intelligent Device’ of some kind, like those portable computers from the Outside World.", NULL);
	add_ending_entry(e, -1, "This one was far more advanced than anything she’d ever seen.", NULL);

	add_ending_entry(e, -1, "It was almost as if it was liquid metal, similar to mercury, but able to take on a solid shape. It cycled through various form factors, like a book, or a phone.", NULL);
	add_ending_entry(e, -1, "Finally, Marisa worked out how to turn it on, and the title of a book appeared on its display…", NULL);
	add_ending_entry(e, -1, "’Practical & Advanced Computational Applications of the Grand Unified Theory’…", NULL);
	add_ending_entry(e, -1, "… ’by Usami Renko”", NULL);

	add_ending_entry(e, 300, "-BAD END-\nTo unlock the Extra Stage, keep trying to reach the end without using a Continue!", NULL);
	track_ending(ENDING_BAD_1);
}

void bad_ending_youmu(Ending *e) {
	add_ending_entry(e, -1, "[The Tower of Babel]", NULL);
	add_ending_entry(e, -1, "The greatest swordswoman of Gensōkyō and the Netherworld, Konpaku Yōmu, stood over a makeshift grave, praying.", NULL);
	add_ending_entry(e, -1, "No, she hadn’t slain her opponent. It was a little more troubling than that.", NULL);

	add_ending_entry(e, -1, "Yuyuko: ”Tell me again… she simply vanished? In mid-air?”", NULL);
	add_ending_entry(e, -1, "Yōmu: ”Yes. One moment she was there, and the next, she was not.”", NULL);
	add_ending_entry(e, -1, "Yōmu: ”The vampire from before also disappeared, in addition to all the loitering fairies…”", NULL);
	add_ending_entry(e, -1, "Yōmu: ”It was as if everything within it ceased to exist entirely.”", NULL);

	add_ending_entry(e, -1, "The Tower had ceased functioning, returing the land and yōkai around it to some semblance of status quo, despite its ever-imposing presence.", NULL);
	add_ending_entry(e, -1, "But the instigators were nowhere to be found, even after much searching.", NULL);

	add_ending_entry(e, -1, "Yōmu’s fearless audacity had dissipated as well, replaced with a sense of loss.", NULL);

	add_ending_entry(e, -1, "Yōmu: ”Thus, I must assume that I have caused their deaths in some way, despite my best efforts.”", NULL);
	add_ending_entry(e, -1, "Yōmu: ”That woman, Elly… we knew each other for such a short time, yet her passion and fury affected me so deeply.”", NULL);
	add_ending_entry(e, -1, "Yuyuko: ”Hmmmm. I didn’t sense her passing through the Netherworld, so I doubt she’s truly died.”", NULL);

	add_ending_entry(e, -1, "Yuyuko: ”Although this ’parallel universes’ stuff is all beyond me, hehehe.”", NULL);
	add_ending_entry(e, -1, "Yōmu took a knee in front of Yuyuko, her hand on the hilt of her blade, the Hakurōken.", NULL);
	add_ending_entry(e, -1, "Yōmu: ”Thank you for entertaining your humble servant’s odd request, Lady Yuyuko.”", NULL);
	add_ending_entry(e, -1, "Yuyuko: ”Oh my, think nothing of it! You did great!”", NULL);

	add_ending_entry(e, -1, "Yuyuko: ”But since we’re in the neighbourhood… do you think the kappa are running one of their famous grill stands?”", NULL);
	add_ending_entry(e, -1, "Yuyuko: ”As a celebration for the incident being resolved, perhaps…?”", NULL);
	add_ending_entry(e, -1, "Yuyuko: ”We could go on a fun date! And I’m feeling a bit peckish…”", NULL);

	add_ending_entry(e, -1, "Nitori’s ’Gaze Upon The Amazing Tower (from A Safe Distance)’ promotional campaign, to encourage locals to visit Yōkai Mountain again after being scared off by the Tower, was in full swing.", NULL);
	add_ending_entry(e, -1, "Although, Yōmu found it difficult to enjoy herself.", NULL);
	add_ending_entry(e, -1, "Somewhere, somehow, she hoped the furiously righteous scythe-bearer was still out there, waiting for their rematch…", NULL);

	add_ending_entry(e, 300, "-BAD END-\nTo unlock the Extra Stage, keep trying to reach the end without using a Continue!", NULL);
	track_ending(ENDING_BAD_2);
}

void bad_ending_reimu(Ending *e) {
	add_ending_entry(e, -1, "[The Hakurei Shrine]", NULL);
	add_ending_entry(e, -1, "The shrine at the border of fantasy and reality.", NULL);
	add_ending_entry(e, -1, "The Tower had powered down, yet the instigators were nowhere to be found.", NULL);
	add_ending_entry(e, -1, "Reimu returned to her shrine, unsure if she had done anything at all.", NULL);
	add_ending_entry(e, -1, "The Gods and yōkai of the Mountain shuffled about the shrine quietly, with bated breath.", NULL);

	add_ending_entry(e, -1, "Reimu sighed, and made her announcement.", NULL);
	add_ending_entry(e, -1, "Reimu: ”Listen up! I stopped the spread of madness, but until further notice, Yōkai Mountain is off limits!”", NULL);

	add_ending_entry(e, -1, "Nobody was pleased with that.", NULL);
	add_ending_entry(e, -1, "The Moriya Gods were visibly shocked.", NULL);
	add_ending_entry(e, -1, "The kappa demanded that they be able to fetch their equipment and tend to their hydroponic cucumber farms.", NULL);
	add_ending_entry(e, -1, "The tengu furiously scribbled down notes, once again as if they’d had the scoop of the century, before realizing what had just been said, and got up in arms too.", NULL);

	add_ending_entry(e, -1, "Once Reimu had managed to placate the crowd, she sat in the back of the Hakurei Shrine, bottle of sake in hand.", NULL);
	add_ending_entry(e, -1, "She didn’t feel like drinking, however. She nursed it without even uncorking it.", NULL);

	add_ending_entry(e, -1, "Yukari: ”A little unsatisfying, isn’t it?”", NULL);
	add_ending_entry(e, -1, "Reimu was never surprised by Yukari’s sudden appearances anymore. Ever since she had gained ’gap’ powers of her own, Reimu could feel Yukari’s presence before she even appeared.", NULL);
	add_ending_entry(e, -1, "Yukari: ”My oh my, it’s unlike you to stop until everything is business as usual, Reimu.”", NULL);
	add_ending_entry(e, -1, "Yukari: ”Depressed?”", NULL);

	add_ending_entry(e, -1, "Reimu: ”Yeah. That place sucked.”", NULL);
	add_ending_entry(e, -1, "Yukari: ”Perhaps I should’ve gone with you.”", NULL);
	add_ending_entry(e, -1, "Reimu: ”Huh? Why? Do you wanna gap the whole thing out of Gensōkyō?”", NULL);
	add_ending_entry(e, -1, "Yukari: ”That might be a bit much, even for me, my dear.”", NULL);

	add_ending_entry(e, -1, "Reimu: ”Whatever that ’madness’ thing was, it was getting to me, too. It made me feel frustrated and… lonely?”", NULL);
	add_ending_entry(e, -1, "Reimu: ”But why would I feel lonely? It doesn’t make any sense.”", NULL);
	add_ending_entry(e, -1, "Yukari: ”So, what will you do?”", NULL);
	add_ending_entry(e, -1, "Reimu: ”Find a way to get rid of it, eventually. I’m sure there’s an answer somewhere out there…”", NULL);

	add_ending_entry(e, -1, "Reimu: ”Maybe Alice or Yuuka know something.”", NULL);
	add_ending_entry(e, -1, "Yukari smiled.", NULL);
	add_ending_entry(e, -1, "Yukari: ”Keep trying until you figure it out, okay?”", NULL);

	add_ending_entry(e, 300, "-BAD END-\nTo unlock the Extra Stage, keep trying to reach the end without using a Continue!", NULL);
	track_ending(ENDING_BAD_3);
}

void good_ending_marisa(Ending *e) {
	add_ending_entry(e, -1, "[The Moriya Shrine]", NULL);
	add_ending_entry(e, -1, "A rival shrine at the top of Yōkai Mountain.", NULL);
	add_ending_entry(e, -1, "Once defeated, the instigators agreed to power down the Tower of Babel.", NULL);
	add_ending_entry(e, -1, "Elly and Kurumi were made to scrub the decks of the Moriya Shrine, which had become filthy from the local fairies partying and dueling non-stop during the Incident…", NULL);
	add_ending_entry(e, -1, "… and from some rapidly-diminishing demonstrations from a newly-formed ”political bloc”, the ”Insect Party.”", NULL);

	add_ending_entry(e, -1, "Sanae: ”—which had a brand new math co-processor letting it have advanced vector graphics processing capabilities. Combined with other improvements, like its DVD drive—", NULL);
	add_ending_entry(e, -1, "Elly: ”Ah, yes, I see facinating, wow. incredible.”", NULL);
	add_ending_entry(e, -1, "Kanako: ”You two having fun?”", NULL);
	add_ending_entry(e, -1, "Elly: ”… y-yes, ma’am.”", NULL);

	add_ending_entry(e, -1, "Kanako: ”Seems like it’s taking a bit more time for the madness to wear off on her. How’s all that ’knowledge’ working out for you?”", NULL);
	add_ending_entry(e, -1, "Sanae: ”I remembered way more about retro video game consoles than I wanted to! And now *you’ve* gotta deal with it!”", NULL);
	add_ending_entry(e, -1, "Elly: *sobs*", NULL);

	add_ending_entry(e, -1, "All said and done, the newcomers seemed to calm down once the effects of the Tower had worn off, just like everyone else.", NULL);
	add_ending_entry(e, -1, "Soon after, Marisa arrived, hopping down off of her broom with a bag of goodies.", NULL);

	add_ending_entry(e, -1, "Elly: ”Ah, Ms. Kirisame. Hello again.”", NULL);
	add_ending_entry(e, -1, "Marisa: ”Yo, Elly.”", NULL);
	add_ending_entry(e, -1, "Elly: ”Do you… need something from me?”", NULL);
	add_ending_entry(e, -1, "Marisa: ”Heck yeah I do! A drinkin’ partner!”", NULL);

	add_ending_entry(e, -1, "Elly: ”Isn’t it a bit early for that?”", NULL);
	add_ending_entry(e, -1, "Marisa: ”Oh c’mon, it’s noon somewhere in the infinite multiverse, ain’t it?”", NULL);
	add_ending_entry(e, -1, "Elly laughed, and went along with it. She needed an excuse for a break anyways.", NULL);
	add_ending_entry(e, -1, "And so, they drank, and caught up on alternate worlds, strange timelines, and the strangeness of their circumstances.", NULL);

	add_ending_entry(e, -1, "Marisa: ”I mean, ’course ya lost! Don’t y’know about Isaac Newton?”", NULL);
	add_ending_entry(e, -1, "Elly: ”The famous European mathematician? I studied his works, yes. But what about him?”", NULL);
	add_ending_entry(e, -1, "Marisa: ”He was an Alchemist!”", NULL);
	add_ending_entry(e, -1, "Elly: ”He was a… what?!”", NULL);

	add_ending_entry(e, -1, "Marisa: ”Yup. Even got a copy of his occult writings back home.”", NULL);
	add_ending_entry(e, -1, "Marisa: ”Pretty cool guy, was into a lotta wild stuff.”", NULL);
	add_ending_entry(e, -1, "Elly deflated against the soft grass behind her, philosophically defeated.", NULL);
	add_ending_entry(e, -1, "Elly: ”That… *is* kind of funny, actually.”", NULL);

	add_ending_entry(e, -1, "Marisa: ”Oh cheer up, will ya? Y’all’ll get to chill out here for as long as ya like.”", NULL);
	add_ending_entry(e, -1, "Elly: ”But what if this Gensōkyō collapses, too…?”", NULL);
	add_ending_entry(e, -1, "Marisa: ”It could! The barrier could fall tomorrow! But what’s the point in worryin’ about all that?”", NULL);
	add_ending_entry(e, -1, "Marisa: ”Right now, I say we enjoy what we got!”", NULL);

	add_ending_entry(e, -1, "Elly: ”I suppose you’re right.”", NULL);
	add_ending_entry(e, -1, "Elly: ”I have to ask, though… how did you remember me?”", NULL);
	add_ending_entry(e, -1, "Marisa: ”Oh, that’s easy. Mushrooms!”", NULL);
	add_ending_entry(e, -1, "Elly: ”Mush… rooms? As in, mycelium?”", NULL);

	add_ending_entry(e, -1, "Marisa: ”Yeah! A while ago, I was cookin’ up some especially powerful ones I found in a cave…”", NULL);
	add_ending_entry(e, -1, "Marisa: ”And then, BOOM, there was a small explosion!”", NULL);
	add_ending_entry(e, -1, "Marisa: ”Before I knew it was seein’ all sorts’a ’versions’ of myself out there. My lives were flashin’ before my eyes!”", NULL);
	add_ending_entry(e, -1, "Marisa: ”Anyway, took me a bit, but I kinda remembered that vampire girl Kurumi, and when I saw ya, and I was like, ”whoa, really?! For real?!””", NULL);

	add_ending_entry(e, -1, "Elly paused, and then laughed at the ridiculousness of it all.", NULL);
	add_ending_entry(e, -1, "Elly: ”That’s it, then? But I suppose it makes sense in a place like Gensōkyō…”", NULL);
	add_ending_entry(e, -1, "Elly: ”You were such a child when I met you. It’s odd to see you all grown up, even if it’s another you entirely.”", NULL);
	add_ending_entry(e, -1, "Marisa: ”Yup, humans grow up. I’m completely ordinary, after all.”", NULL);

	add_ending_entry(e, -1, "Some wondered if the Tower would stand out on Gensōkyō’s skyline for eternity.", NULL);
	add_ending_entry(e, -1, "Some even questioned if its power could be used to serve the greater good.", NULL);
	add_ending_entry(e, -1, "And a select few considered what they’d do if the Tower were to ever spring to life again…", NULL);
	add_ending_entry(e, 300, "-GOOD ENDING-\nOne Credit Clear Achieved!\nExtra Stage has been unlocked! Do you have what it takes to face the true nature of reality?", NULL);
	track_ending(ENDING_GOOD_1);
}

void good_ending_youmu(Ending *e) {
	add_ending_entry(e, -1, "[The Moriya Shrine]", NULL);
	add_ending_entry(e, -1, "A rival shrine at the top of Yōkai Mountain.", NULL);
	add_ending_entry(e, -1, "The Tower of Babel stood silently, as if a sleeping giant.", NULL);
	add_ending_entry(e, -1, "Below, at the not-so-humble Moriya Shrine, two blademasters were exchanging the ways of their craft.", NULL);

	add_ending_entry(e, -1, "Elly: ”I’m sure you’re aware that the classic scythe such as this was primarily used by farmers, as a makeshift weapon—”", NULL);
	add_ending_entry(e, -1, "Yōmu: ”Yes, of course. Some of the best martial arts evolved from the practices of common folk—”", NULL);

	add_ending_entry(e, -1, "After the incident, Elly and Kurumi agreed to set the Tower into a low-power state.", NULL);
	add_ending_entry(e, -1, "Their dispositions softened considerably, and they apologized profusely for everything they had done.", NULL);
	add_ending_entry(e, -1, "Although, oddly, Yōmu retained much of the confidence she had gained in the incident, despite the Tower’s presence no longer having any effect.", NULL);

	add_ending_entry(e, -1, "Elly: ”This Gensōkyō is so different compared to my own.”", NULL);
	add_ending_entry(e, -1, "Yōmu: ”What was your Gensōkyō like?”", NULL);
	add_ending_entry(e, -1, "Elly: ”Mostly, a lot smaller. It felt more like a road-side food stand.”", NULL);
	add_ending_entry(e, -1, "Yōmu: ”Hmm, fascinating…”", NULL);

	add_ending_entry(e, -1, "Elly: ”Well, I ought to return to my duties at the shrine.”", NULL);
	add_ending_entry(e, -1, "Elly: ”I hope the green-haired shrine maiden doesn’t talk my ear off about obsolete entertainment technology again…”", NULL);
	add_ending_entry(e, -1, "Yōmu: ”I commend you for complying with your penance, despite its… cruel hardships.”", NULL);
	add_ending_entry(e, -1, "The pair of them laughed.", NULL);

	add_ending_entry(e, -1, "Elly: ”I know I said this before, but I still feel like a fool.”", NULL);
	add_ending_entry(e, -1, "Elly: ”I was miserable about my home, but that was going too far…”", NULL);
	add_ending_entry(e, -1, "Elly: ”It seems like the one most overcome by madness was myself.”", NULL);
	add_ending_entry(e, -1, "Elly: ”I’m still truly sorry for what I did.”", NULL);

	add_ending_entry(e, -1, "Yōmu put a hand on Elly’s shoulder and smiled.", NULL);
	add_ending_entry(e, -1, "Yōmu: ”Your motivations were righteous, even if your methods were not.”", NULL);
	add_ending_entry(e, -1, "Yōmu: ”Now that you’ve recognized that, you are welcome here, like anyone else.”", NULL);
	add_ending_entry(e, -1, "Yōmu: ”Tomorrow, provided Lady Yuyuko has no business for me, I would like to show you what the Netherworld and Gensōkyō have to offer!", NULL);
	add_ending_entry(e, -1, "Yōmu: ”I’m feeling generous, so it shall be my treat!”", NULL);

	add_ending_entry(e, -1, "Elly happily agreed.", NULL);
	add_ending_entry(e, -1, "The pair of them had become fast friends. Practically inseparable, even.", NULL);
	add_ending_entry(e, -1, "As they spent many evenings dueling, practicing, and drinking merrily, many folks commented on how they had never seen Yōmu so thrilled.", NULL);
	add_ending_entry(e, -1, "Nobody had figured out how to remove the Tower, and most were too anxious to even think about it.", NULL);

	add_ending_entry(e, -1, "With such an enormous visage, many wondered if the Tower was truly inert like the heroine claimed it to be…", NULL);
	add_ending_entry(e, -1, "Meanwhile, in the Netherworld, a certain gap yōkai and her ghostly companion discussed details of ancient future’s past.", NULL);

	add_ending_entry(e, 300, "-GOOD ENDING-\nOne Credit Clear Achieved!\nExtra Stage has been unlocked! Do you have what it takes to face the true nature of reality?", NULL);
	track_ending(ENDING_GOOD_2);
}

void good_ending_reimu(Ending *e) {
	add_ending_entry(e, -1, "[The Moriya Shrine]", NULL);
	add_ending_entry(e, -1, "A rival shrine at the top of Yōkai Mountain.", NULL);

	add_ending_entry(e, -1, "Reimu: ”Listen, Sanae, I don’t know what a ’speedrun’ is, and I really don’t care either!”", NULL);
	add_ending_entry(e, -1, "Sanae: ”You’re such a stick in the mud, Reimu! Listen, you like resolving incidents as quickly as possible, right?”", NULL);
	add_ending_entry(e, -1, "Sanae: ”It’s like that, except with video games!”", NULL);
	add_ending_entry(e, -1, "Reimu: ”I don’t know anything about ’video games’ either!”", NULL);

	add_ending_entry(e, -1, "The Tower had been shut down, ending the crisis of madness sweeping the land.", NULL);
	add_ending_entry(e, -1, "Most everyone had returned to normal, although a few odd souls lamented the loss of knowledge.", NULL);
	add_ending_entry(e, -1, "Kurumi and Elly had been conscripted into sweeping the paths of the Moriya Shrine as penance for their actions.", NULL);
	add_ending_entry(e, -1, "With the occasional odd request from the Moriya Shrine Gods, of course…", NULL);

	add_ending_entry(e, -1, "Kanako: ”I can’t believe it! Is it really that simple?!”", NULL);
	add_ending_entry(e, -1, "Elly: ”Y-yes, it’s just a matter of changing that coefficient there, and shaping the magnetic containment field using—”", NULL);
	add_ending_entry(e, -1, "Kanako: ”Of course! I was such a fool! I can’t believe I ever bothered attempting to harness the Yatagarasu!”", NULL);
	add_ending_entry(e, -1, "Kanako: ”At last, I have the final key to modernizing Gensōkyō, and preventing the energy crisis from reaching us! Fusion power is now attainable!”", NULL);

	add_ending_entry(e, -1, "Elly: ”A-are you sure you’re well, ma’am?”", NULL);
	add_ending_entry(e, -1, "Suwako: ”Heh heh, don’t worry. This is as normal as it gets for Kanako.”", NULL);
	add_ending_entry(e, -1, "Elly: ”Normal? Really?”", NULL);
	add_ending_entry(e, -1, "Suwako: ”Don’t you think she’s cute when she’s like this?”", NULL);

	add_ending_entry(e, -1, "Reimu was spending more time on Yōkai Mountain, claiming she was ”keeping watch.”", NULL);
	add_ending_entry(e, -1, "For some reason, she seemed to have a difficult time settling down from this incident in particular.", NULL);
	add_ending_entry(e, -1, "Reimu: ”Hey, Elly.”", NULL);
	add_ending_entry(e, -1, "Elly: ”Ah, yes? Do you need something from me?”", NULL);

	add_ending_entry(e, -1, "Reimu: ”I’m surprised you’re still not pissed off at everyone. Not even a little bit.”", NULL);
	add_ending_entry(e, -1, "Reimu: ”You were shouting like a dictator up there.”", NULL);
	add_ending_entry(e, -1, "Elly: ”It seems that the Tower got to me in the end, too.”", NULL);
	add_ending_entry(e, -1, "Elly: ”I’m still sad about my own Gensōkyō, but to go that far…”", NULL);

	add_ending_entry(e, -1, "Reimu: ”What a weird machine. I wonder what the kappa would think of it.”", NULL);
	add_ending_entry(e, -1, "Suwako: ”The kappa won’t touch the thing. And even Kanako’s talked herself out of it.”", NULL);
	add_ending_entry(e, -1, "Reimu: ”Ah, whatever. As long as it stays quiet, it’s not even the weirdest thing in Gensōkyō.”", NULL);
	add_ending_entry(e, -1, "Reimu: ”Still odd how you two were forgotten from Gensōkyō, though…”", NULL);

	add_ending_entry(e, -1, "With the odd feeling gone, everyone seemed to relax more than usual.", NULL);
	add_ending_entry(e, -1, "The Tower of Babel continued to loom over Yōkai Mountain, as life returned to normal.", NULL);
	add_ending_entry(e, -1, "Even as some grew to accept it, there were frequent mutterings over potentially getting rid of the Tower entirely.", NULL);
	add_ending_entry(e, -1, "Although, nobody had any clue how, except, perhaps, a certain violet gap yōkai…", NULL);

	add_ending_entry(e, 300, "-GOOD ENDING-\nOne Credit Clear Achieved!\nExtra Stage has been unlocked! Do you have what it takes to face the true nature of reality?", NULL);
	track_ending(ENDING_GOOD_3);
}

static void init_ending(Ending *e) {
	if(global.plr.continues_used) {
		global.plr.mode->character->ending.bad(e);
	} else {
		global.plr.mode->character->ending.good(e);
		add_ending_entry(e, 400, "-UNDER CONSTRUCTION-\nExtra Stage coming soon! Please wait warmly for it!", NULL);
	}

	add_ending_entry(e, 400, "", NULL); // this is important
	e->duration = 1<<23;
}

static void free_ending(Ending *e) {
	int i;
	for(i = 0; i < e->count; i++)
		free(e->entries[i].msg);
	free(e->entries);
}

static void ending_draw(Ending *e) {
	float s, d;

	int t1 = global.frames-e->entries[e->pos].time;
	int t2 = e->entries[e->pos+1].time-global.frames;

	d = 1.0/ENDING_FADE_TIME;

	if(t1 < ENDING_FADE_TIME)
		s = clamp(d*t1, 0.0, 1.0);
	else
		s = clamp(d*t2, 0.0, 1.0);

	r_clear(CLEAR_ALL, RGBA(0, 0, 0, 1), 1);

	r_color4(s, s, s, s);

	if(e->entries[e->pos].sprite) {
		r_draw_sprite(&(SpriteParams) {
			.sprite_ptr = e->entries[e->pos].sprite,
			.pos = { SCREEN_W/2, SCREEN_H/2 },
			.shader = "sprite_default",
			.color = r_color_current(),
		});
	}

	r_shader("text_default");
	text_draw_wrapped(e->entries[e->pos].msg, SCREEN_W * 0.85, &(TextParams) {
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
	if(
		e->pos < e->count-1 &&
		e->entries[e->pos].time + ENDING_FADE_TIME < global.frames &&
		global.frames < e->entries[e->pos+1].time - ENDING_FADE_TIME
	) {
		e->entries[e->pos+1].time = global.frames+(e->pos != e->count-2)*ENDING_FADE_TIME;
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

	if(e->pos < e->count-1 && global.frames >= e->entries[e->pos+1].time) {
		e->pos++;
		if(e->pos == e->count-1) {
			fade_bgm((FPS * ENDING_FADE_OUT) / 4000.0);
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

	if(e->pos < e->count-1)
		ending_draw(e);

	draw_transition();
	return RFRAME_SWAP;
}

static void ending_loop_end(void *ctx) {
	Ending *e = ctx;
	CallChain cc = e->cc;
	free_ending(e);
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
	start_bgm("ending");

	eventloop_enter(e, ending_logic_frame, ending_render_frame, ending_loop_end, FPS);
}
