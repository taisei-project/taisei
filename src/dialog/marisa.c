/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "dialog_macros.h"

/*
 * Stage 1
 */

DIALOG_TASK(marisa, Stage1PreBoss) {
	DIALOG_BEGIN(Stage1PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(cirno);
	HIDE(cirno);

	FACE(marisa, puzzled);
	MSG(marisa, "Aw, snow again? I just put away my winter coat.");
	FACE(marisa, sweat_smile);
	MSG(marisa, "…even though it’s half-way through summer already.");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(cirno, 180, "Nice, right?");

	SHOW(cirno);
	FACE(cirno, normal);
	MSG(cirno, "It’s my snow. I did it!");

	FACE(marisa, normal);
	MSG(marisa, "Mind if I borrow some money for the drycleanin’ bill?");
	FACE(marisa, sweat_smile);
	MSG(marisa, "I was gonna get it done in the fall, but now…");

	TITLE(cirno, "Cirno", "Thermodynamic Ice Fairy");
	MSG(cirno, "No, but I can lend you some ice cream!");
	EVENT(music_changes);
	FACE(cirno, angry);
	MSG(cirno, "After I turn YOU into ice cream!");

	FACE(marisa, smug);
	MSG(marisa, "I prefer shaved ice, myself.");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage1PostBoss) {
	DIALOG_BEGIN(Stage1PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(cirno);
	VARIANT(cirno, defeated);
	FACE(cirno, defeated);

	FACE(marisa, smug);
	MSG(marisa, "Why don’t you go play with your new friend, the one from Hell?");
	MSG(marisa, "Maybe then it’d be more of a fair fight.");

	MSG(cirno, "Ouch…");
	MSG(cirno, "I-I mean, last time she lost! And next time, you will too!");

	MSG(marisa, "Sure, sure.");

	DIALOG_END();
}

/*
 * Stage 2
 */

DIALOG_TASK(marisa, Stage2PreBoss) {
	DIALOG_BEGIN(Stage2PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(hina);
	HIDE(hina);
	FACE(marisa, unamused);
	MSG(marisa, "Geez, this place is kinda sad now.");
	MSG(marisa, "What could be causin’ the yōkai to get all weird, anyways?");

	EVENT(boss_appears);
	WAIT(60);
	SHOW(hina);
	FACE(hina, normal);
	MSG(hina, "Why hello, Ms. Kirisame. It’s been quite a while.");
	MSG(hina, "You seem to be as reckless as always. I recommend turning back.");

	FACE(marisa, happy);
	MSG(marisa, "As always, my reputation as an Incident Solver is at stake here, so I’ll be ignorin’ that advice.");
	FACE(marisa, normal);
	MSG(marisa, "Thanks, though. See ya.");

	TITLE(hina, "Kagiyama Hina", "Gyroscopic Pestilence God");
	FACE(hina, serious);
	MSG(hina, "Oh my… you’re quite the rebellious one, aren’t you?");
	MSG(hina, "Such confidence, ignoring my warning like that.");

	FACE(marisa, happy);
	MSG(marisa, "I’ll take that as a compliment.");

	EVENT(music_changes);
	MSG(hina, "And do you know what happens to naughty girls who don’t listen to their elders?");
	MSG(hina, "They get showered in misfortune for years!");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage2PostBoss) {
	DIALOG_BEGIN(Stage2PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(hina);
	VARIANT(hina, defeated);
	FACE(hina, defeated);
	FACE(marisa, unamused);
	MSG(marisa, "Y’know, you’re way more snippy than usual today, Hina.");
	MSG(marisa, "It’s kinda annoyin’, honestly.");

	MSG(hina, "H-how mean…");
	MSG(hina, "Don’t you see I was just trying to keep you safe…?");

	MSG(marisa, "Oi, don’t get all motherly on me now.");
	MSG(marisa, "I’ve had enough of that for a lifetime.");

	DIALOG_END();
}

/*
 * Stage 3
 */

DIALOG_TASK(marisa, Stage3PreBoss) {
	DIALOG_BEGIN(Stage3PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(wriggle);
	HIDE(wriggle);
	FACE(marisa, normal);
	MSG(marisa, "Everyone’s more reckless today. What gives?");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(wriggle, 180, "Wait ‘til you see their leader!");
	SHOW(wriggle);

	MSG(marisa, "Yeah, I’m gonna have to ask their leader a bunch‘a questions for sure.");

	MSG(wriggle, "Feel free to go ahead. I’m listening.");

	MSG(marisa, "Oh, you’re her messenger?");
	FACE(marisa, happy);
	MSG(marisa, "Mind passin’ somethin’ along?");

	TITLE(wriggle, "Wriggle Nightbug", "Insect Rights Activist");
	FACE(wriggle, outraged);
	MSG(wriggle, "Um, OBVIOUSLY I’m not the messenger!");
	FACE(wriggle, proud);
	MSG(wriggle, "Yeah, I’m the great leader of Insectkind!");

	FACE(marisa, smug);
	MSG(marisa, "Great leader? What, did the insects unionize or somethin’?");

	FACE(wriggle, outraged);
	MSG(wriggle, "No! I told you, I’m the leader! The mastermind!");
	FACE(wriggle, calm);
	MSG(wriggle, "So, what’s your question?");

	FACE(marisa, normal);
	MSG(marisa, "Hmm, guess I forgot.");
	MSG(marisa, "What’s yer deal, anyways?");

	MSG(wriggle, "We insects have three demands:");
	MSG(wriggle, "First: say nay to insect spray!");
	MSG(wriggle, "Second: swat the swatters!");
	MSG(wriggle, "Third: the right to bite!");
	MSG(wriggle, "Fourth—");

	FACE(marisa, unamused);
	MSG(marisa, "Okay okay, this is great ‘n all, but I’ve honestly got more important stuff to do.");
	MSG(marisa, "… and ya don’t got a clue about what’s goin’ on, do ya?");

	EVENT(music_changes);
	FACE(wriggle, proud);
	MSG(wriggle, "Maybe I know more than you think.");
	FACE(wriggle, outraged);
	MSG(wriggle, "But it’s NOTHING I’d tell an usurper of Insectkind’s glory!");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage3PostBoss) {
	DIALOG_BEGIN(Stage3PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(wriggle);
	VARIANT(wriggle, defeated);
	FACE(wriggle, defeated);

	FACE(marisa, smug);
	MSG(marisa, "Tougher than usual, but not tough enough.");
	MSG(marisa, "Where’d ya pick up those new moves, anyway?");

	MSG(wriggle, "Aw! It was um… the great vision of our glorious Insect past!");
	MSG(wriggle, "It was in the age of Car—… Carb—… the age of Carbs!");

	FACE(marisa, puzzled);
	MSG(marisa, "‘Glorious insect past’? ‘Carbs’?");
	FACE(marisa, normal);
	MSG(marisa, "Oh, ya must mean the Carboniferous Period. With the giant bugs ‘n all that.");
	MSG(marisa, "I remember that doctor in the bamboo forest goin’ off about it once.");
	MSG(marisa, "Only time I’d seen her so excited…");

	MSG(wriggle, "Yeah, that!");
	MSG(wriggle, "Do you want to be an ally in our glorious conquest…?");

	FACE(marisa, smug);
	MSG(marisa, "Maybe later.");
	FACE(marisa, happy);
	MSG(marisa, "Probably never.");

	DIALOG_END();
}

/*
 * Stage 4
 */

DIALOG_TASK(marisa, Stage4PreBoss) {
	DIALOG_BEGIN(Stage4PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(kurumi);
	HIDE(kurumi);

	FACE(marisa, surprised);
	MSG(marisa, "Oh boy, a mysterious mansion!");
	FACE(marisa, smug);
	MSG(marisa, "… how generic can you get?");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(kurumi, 180, "And to top it off, a western witch!");
	SHOW(kurumi);
	MSG(kurumi, "Seen it a thousand times already. It’s such a boring motif.");

	FACE(marisa, happy);
	MSG(marisa, "Right? But I make up for it with my devilish charm.");

	FACE(kurumi, dissatisfied);
	MSG(kurumi, "But that theme is so overdone…");
	FACE(kurumi, normal);
	MSG(kurumi, "What’s next? A maid?");
	FACE(marisa, normal);
	MSG(kurumi, "A little girl with almost no dialogue who becomes a fan-favourite anyways?");
	MSG(kurumi, "How about one that’ll have her personality massacred by the fans?");

	FACE(marisa, smug);
	MSG(marisa, "Oh, I wouldn’t know anythin’ about that.");

	MSG(kurumi, "Maybe I’ve been reading too much manga that those tengu churn out.");

	FACE(marisa, puzzled);
	MSG(marisa, "The tengu write manga now?");
	FACE(marisa, sweat_smile);
	MSG(marisa, "Writin’ pure ficiton is more honest for them, I guess.");

	TITLE(kurumi, "Kurumi", "High-Society Phlebotomist");
	FACE(kurumi, normal);
	MSG(kurumi, "Yeah, duh?");
	MSG(kurumi, "Of course, most of it is garbage that only weirdos like!");
	FACE(kurumi, dissatisfied);
	MSG(kurumi, "And don’t EVEN get me started on certain character designs! So gross!");

	FACE(marisa, normal);
	MSG(marisa, "Maybe they oughta hire someone with some fashion sense.");

	FACE(kurumi, normal);
	MSG(kurumi, "That’d be me, then!");
	MSG(kurumi, "My ideas would surely make a splash, if they’d recognize my talent!");

	FACE(marisa, unamused);
	MSG(marisa, "Eh? Maybe…");

	FACE(kurumi, dissatisfied);
	MSG(kurumi, "Huh?!");

	FACE(marisa, smug);
	MSG(marisa, "Your outfit’s missin’ somethin’. Fallen on hard times, maybe?");

	FACE(kurumi, tsun_blush);
	MSG(kurumi, "H-Hmph! I could say the same about you!");
	FACE(kurumi, normal);
	MSG(kurumi, "Seriously, it’s just a plain generic witch outfit!");

	FACE(marisa, normal);
	MSG(marisa, "My petticoats are legendary, y’know.");
	FACE(marisa, happy);
	MSG(marisa, "Girls across Gensōkyō talk about ‘em.");
	FACE(marisa, normal);
	MSG(marisa, "In fact, there’s a few of ‘em, uh, deeper inside this mansion.");
	FACE(marisa, happy);
	MSG(marisa, "So I’ll be headin’ in, then.");

	EVENT(music_changes);
	FACE(kurumi, tsun);
	MSG(kurumi, "Oh, I’m not letting you go without a fight!");
	FACE(kurumi, normal);
	MSG(kurumi, "Let’s see who’s the most fashionable here, witch!");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage4PostBoss) {
	DIALOG_BEGIN(Stage4PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(kurumi);
	VARIANT(kurumi, defeated);
	FACE(kurumi, defeated);

	FACE(marisa, smug);
	MSG(marisa, "Whoever has the best petticoat wins. It’s a scientific fact.");
	FACE(marisa, sweat_smile);
	MSG(marisa, "… but since when do I care about scientific facts?");

	MSG(kurumi, "W-wait, what was I doing again?");
	MSG(kurumi, "(Y-yeah, that’s it, play it off as amnesia…");
	MSG(kurumi, "Uuu, I don’t remember anything at all!!");
	MSG(kurumi, "(But, hmm… haven’t I seen this nerdy-looking girl before…?)");

	FACE(marisa, normal);
	MSG(marisa, "Ah, sorry, sorry. I think ya slipped ‘n hit your head.");
	MSG(marisa, "I’m Gensōkyō’s… petticoat inspector.");
	MSG(marisa, "I’m here to inspect the petticoats in this here mansion.");
	FACE(marisa, happy);
	MSG(marisa, "Ya were just lettin’ me in, so I’ll be off, then.");

	FACE(kurumi, normal);
	MSG(kurumi, "Uh, I kinda doubt that…");
	FACE(kurumi, defeated);
	MSG(kurumi, "Not that I can stop you, anyways…");
	FACE(marisa, normal);
	MSG(kurumi, "Ugh, I didn’t sign up to get beaten like this!");

	FACE(marisa, puzzled);
	MSG(marisa, "‘Sign up’? I thought you didn’t remember anything.");

	FACE(kurumi, tsun_blush);
	MSG(kurumi, "N-Nevermind!");

	DIALOG_END();
}

/*
 * Stage 5
 */

DIALOG_TASK(marisa, Stage5PreBoss) {
	DIALOG_BEGIN(Stage5PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(iku);
	HIDE(iku);

	FACE(marisa, inquisitive);
	MSG(marisa, "This place is fascinatin’!");
	MSG(marisa, "What kinda energy source would they even use for this?!");
	MSG(marisa, "I gotta tell Patchy!");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(iku, 180, "I would not recommend getting too enamoured, young witch.");
	SHOW(iku);
	MSG(iku, "Regardless, I’m surprised to see you here. How have you been?");

	MSG(marisa, "What grimoire would this be in…?");
	MSG(marisa, "Hmm… probably a major illusion of some kind…?");
	MSG(marisa, "That’d be easier than making the bricks weightless, ‘n it’s a good disguise…");

	TITLE(iku, "Nagae Iku", "Fulminologist of the Heavens");
	FACE(iku, smile);
	MSG(iku, "U-um, pardon me. Are you conversing with yourself?");

	FACE(marisa, normal);
	MSG(marisa, "Huh? No, I was askin’ ya a question.");

	MSG(iku, "Would that be your theory about this tower, perhaps?");

	MSG(marisa, "It’d make sense though, right?");
	FACE(marisa, inquisitive);
	MSG(marisa, "It appeared outta nowhere. If it were real brick, the air displacement alone woulda caused a huge noise…");
	MSG(marisa, "And it’d take so much power, too.");
	MSG(marisa, "Definitely a major illusion, then.");

	MSG(iku, "I feel as though the premise of your theory is flawed, unfortunately.");
	FACE(iku, eyes_closed);
	FACE(marisa, normal);
	MSG(iku, "The technology on display here is beyond our current comprehension.");

	FACE(marisa, surprised);
	MSG(marisa, "Technology? Whoa, so yer sayin’ this is REAL brick?!");
	FACE(marisa, normal);
	MSG(marisa, "*tap tap*");
	FACE(marisa, surprised);
	MSG(marisa, "It’s so cold it’s almost creepy, like it was suckin’ magic right outta my fingers.");
	FACE(marisa, inquisitive);
	MSG(marisa, "But that means the tower… has corporeal form?!");
	MSG(marisa, "Dang! Now I GOTTA meet the owner!");
	MSG(marisa, "What grimoires does she got to pull somethin’ like this off?!");

	FACE(iku, smile);
	MSG(iku, "S-she does not use grimoires, or magic.");
	FACE(iku, serious);
	MSG(iku, "She is relying on complex machinery, and likely advanced computational—");

	MSG(marisa, "But then, what’s the power source?");
	MSG(marisa, "Not even nuclear fusion could sustain somethin’ like this!");

	FACE(iku, smile);
	MSG(iku, "I-I am not sure of its exact mechanics—");

	FACE(marisa, happy);
	MSG(marisa, "Where’d y’say she was?");

	FACE(iku, normal);
	MSG(iku, "You mean…?");

	FACE(marisa, inquisitive);
	MSG(marisa, "Yeah, the owner! Upstairs, right?");

	FACE(iku, serious);
	MSG(iku, "Perhaps the thin atmosphere at this elevation has gotten to your head. I should not divulge any further information, for your own protection.");

	FACE(marisa, happy);
	MSG(marisa, "What’s her name?");

	MSG(iku, "Why are you not listening to me? You are not thinking straight!");

	FACE(marisa, inquisitive);
	MSG(marisa, "Do ya got any tips on talkin’ to her, maybe make her loosen up a bit?");
	FACE(marisa, happy);
	MSG(marisa, "I don’t wanna blow my chance.");

	FACE(iku, smile);
	MSG(iku, "I am trying to spare you further peril, yet all you can do is talk over me. Be more considerate.");
	MSG(iku, "I had expectations that you would be able to approach this with a clear head and resolve this incident, but it seems—");

	FACE(marisa, normal);
	MSG(marisa, "Ah, ah, I see. I’ll have to turn on the ol’ Kirisame charm.");
	FACE(marisa, smug);
	FACE(iku, serious);
	MSG(marisa, "I get it, I get it.");
	MSG(marisa, "Haven’t done ‘femme fatale’ in a while, but…");

	EVENT(music_changes);

	MSG(iku, "Enough!");
	MSG(iku, "It is said that specifically applied electro-stimulation can ease the mind.");
	MSG(iku, "Allow me to put that into practice for you!");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage5PostMidBoss) {
	DIALOG_BEGIN(Stage5PostMidBoss);

	ACTOR_LEFT(marisa);
	FACE(marisa, surprised);

	// should be only one message with a fixed 180-frame (3 second) timeout
	MSG_UNSKIPPABLE(marisa, 180, "So this place runs on electricity, eh? Hmm…");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage5PostBoss) {
	DIALOG_BEGIN(Stage5PostBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(iku);
	VARIANT(iku, defeated);
	FACE(iku, defeated);

	FACE(marisa, normal);
	MSG(marisa, "Oof, I hope that didn’t make my hair too frizzy.");
	FACE(marisa, happy);
	MSG(marisa, "Y’know, for my big first impression ‘n all.");

	FACE(iku, eyes_closed);
	MSG(iku, "F-For the last time, she’ll have nothing to teach you…");
	MSG(iku, "Her powers rely on technology and the scientific method. Of that much I am sure.");

	FACE(marisa, unamused);
	MSG(marisa, "The one time Sanae’s not around to geek out about all the sciencey stuff, eh?");
	FACE(iku, normal);
	FACE(marisa, normal);
	MSG(marisa, "Ah well, I know how to operate one of those Intelligent Phones, and that’s plenty advanced, so I should be fine.");
	FACE(marisa, inquisitive);
	MSG(marisa, "By the way, have ya been hearing that strange voice?");

	MSG(iku, "Strange voice?");

	FACE(marisa, smug);
	MSG(marisa, "Yeah, it keeps tellin’ me that I’m not thinkin’ straight or whatever.");
	MSG(marisa, "Probably just those weird madness rays tryin’ to get me.");

	FACE(iku, eyes_closed);
	MSG(iku, "Ugh, you fool.");
	MSG(iku, "But perhaps you are thinking just well enough to resolve this incident after all…");

	FACE(marisa, surprised);
	MSG(marisa, "Ah! There’s that voice again! So strange…");

	DIALOG_END();
}

/*
 * Stage 6
 */

DIALOG_TASK(marisa, Stage6PreBoss) {
	DIALOG_BEGIN(Stage6PreBoss);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(elly);
	HIDE(elly);

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(elly, 180, "I’ve been waiting for you to make your move.");
	SHOW(elly);
	MSG(elly, "Sneaking around as you are, and yet still so loud.");

	FACE(marisa, unamused);
	MSG(marisa, "Hmm…");

	FACE(elly, smug);
	MSG(elly, "Did you really think I wouldn’t notice all the commotion you caused inside the tower?");

	MSG(marisa, "Hmmmmmmm…");

	FACE(elly, angry);
	MSG(elly, "Stop being an insolent brat and speak up!");

	FACE(marisa, unamused);
	MSG(marisa, "… sigh.");

	FACE(elly, normal);
	MSG(elly, "… w-what is it?");

	FACE(marisa, unamused);
	MSG(marisa, "Ya don’t look like a magician after all.");
	MSG(marisa, "I’m pretty disappointed.");
	MSG(marisa, "Honestly, this might be the most disappointing incident yet.");

	TITLE(elly, "Elly", "The Theoretical Reaper");
	MSG(elly, "You’re after… magic?");
	FACE(elly, smug);
	MSG(elly, "Hah! You’ll regret wasting your time on such nonsense.");
	MSG(elly, "Soon, you’ll see the error of—");

	MSG(marisa, "Do ya know how boring it is to be a magician in Gensōkyō these days?");

	FACE(elly, eyes_closed);
	MSG(elly, "I couldn’t possibly care about—");

	MSG(marisa, "Seriously! Recently it’s just been ‘God this’ and ‘Otherworld that’.");
	MSG(marisa, "Reimu’s been pickin’ up some sweet skills, but me? Nothin’!");
	MSG(marisa, "Gods use magic too, but they’re like her! They don’t need books!");
	MSG(marisa, "But I do! And I end up empty handed every dang time!");

	FACE(elly, angry);
	MSG(elly, "I SAID I don’t CARE about your stupid little sorc—");

	MSG(marisa, "I saw this place ‘n I was all, ‘Whoa! Is this all enchantments ‘n stuff?!’");
	FACE(marisa, inquisitive);
	MSG(marisa, "I got really excited!");
	MSG(marisa, "‘Maybe I could get a new trump-card outta this’ is what I thought.");
	FACE(marisa, unamused);
	MSG(marisa, "But obviously yer a shinigami or somethin’. And I didn’t see a library…");
	MSG(marisa, "I guess that oarfish from before was right.");
	MSG(marisa, "Master Spark’s good ‘n all, but it’ll only get me so far in life…");

	FACE(elly, eyes_closed);
	MSG(elly, "…");
	MSG(elly, "Are you finally done?");
	FACE(elly, angry);
	MSG(elly, "Is this some kind of joke?!");
	FACE(elly, shouting);
	MSG(elly, "Are you just trying to mock me like you did the first time we met?!");

	MSG(marisa, "At least that time ya were guardin’ some powerful thingy I coulda used.");
	MSG(marisa, "I’m tryin’ to remember, was that time a bust, too?");
	FACE(marisa, happy);
	MSG(marisa, "At least I met Yūka, right? Did ya know she ended up givin’ me my trademark spell?");
	MSG(marisa, "Not right then, ‘n not voluntarily, but—");

	FACE(elly, angry);
	MSG(elly, "Y-you remember that?! But how—?!");
	MSG(elly, "… ugh, nevermind! It’s not as if you could possibly grasp the Tower’s true power.");
	FACE(elly, smug);
	MSG(elly, "After all, I can already sense your feeble mind succumbing to its might!");

	FACE(marisa, unamused);
	MSG(marisa, "Eh? I’m not depressed or anythin’. I’m just disappointed.");

	FACE(elly, shouting);
	MSG(elly, "I-It’s not about being ‘depressed’! It’s about being overwhelmed with the vast knowledge of the universe!");
	MSG(elly, "Once you realize the true potential of reality, you’ll go mad with knowledge!");
	MSG(elly, "Don’t you see?! Nothing can stop us now! For we are—!");

	FACE(marisa, surprised);
	MSG(marisa, "But I’m already mad with knowledge! Why do ya think I drink all the time?!");
	MSG(marisa, "My mind’s always racin’ with this ‘n that…");
	MSG(marisa, "Ya think this is any different than how I usually live?! Get over y’erself!");
	FACE(marisa, inquisitive);
	MSG(marisa, "Now, I gotta ask… ya really, seriously don’t got a secret library anywhere?");
	MSG(marisa, "A single bookshelf?! A few loose scribbled notes?!");
	MSG(marisa, "Heck, I’ll even take one of those little glowy tablet thingies with all those ‘Pee Dee Effs’ on ‘em! They hurt my dang eyes, but—");

	FACE(elly, eyes_closed);
	MSG(elly, "Since you’re OBVIOUSLY treating this as a joke, I REFUSE to speak with you any longer!");

	FACE(marisa, happy);
	MSG(marisa, "Oh c’mon, wanna catch up on old times?");
	MSG(marisa, "Turn the madness-whatever off ‘n we’ll go out for drinks! There’s this nice new bar in town with a cute—");

	MSG(elly, "There’s no force in this world that would make me turn back now!");
	EVENT(music_changes);
	FACE(elly, smug);
	MSG(elly, "Someone as ‘ordinary’ as you getting in our way is impressive, I’ll give you that much.");
	FACE(elly, angry);
	MSG(elly, "But there’s no place for sorcery in our enlightened vision of Gensōkyō!");
	FACE(elly, shouting);
	MSG(elly, "Now, succumb to the madness!");

	DIALOG_END();
}

DIALOG_TASK(marisa, Stage6PreFinal) {
	DIALOG_BEGIN(Stage6PreFinal);

	ACTOR_LEFT(marisa);
	ACTOR_RIGHT(elly);
	VARIANT(elly, beaten);
	FACE(elly, angry);
	FACE(marisa, unamused);
	MSG(marisa, "Are ya sure ya don’t got a spellbook laying around?! Nothin’ at all?!");
	MSG(elly, "Are you still on about that?! Fine, see what good it does you!");
	FACE(elly, shouting);
	MSG(elly, "Your pitiful magic amounts to nothing against the true nature of reality!");

	DIALOG_END();
}

/*
 * Register the tasks
 */

#define EXPORT_DIALOG_TASKS_CHARACTER marisa
#include "export_dialog_tasks.inc.h"
