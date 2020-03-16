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

DIALOG_TASK(reimu, Stage1PreBoss) {
	PRELOAD {
		PRELOAD_CHAR(reimu) {
			PRELOAD_FACE(normal);
			PRELOAD_FACE(unamused);
			PRELOAD_FACE(sigh);
		}

		PRELOAD_CHAR(cirno) {
			PRELOAD_FACE(normal);
			PRELOAD_FACE(angry);
		}
	}

	// Initialization, must be at the very top (after PRELOAD).
	DIALOG_BEGIN(Stage1PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(cirno);

	// Hide cirno for now, to be revealed later.
	HIDE(cirno);

	// "normal" is the default face.
	// FACE(reimu, normal);

	// FOCUS() to make an actor stand out.
	// Focused actors are fully bright and appear in front of the rest.
	// Only one actor can be focused at a time.
	FOCUS(reimu);

	// Let’s wait a bit, then change Reimu’s expression.
	// All timings are in frames (60 = 1 sec).
	WAIT_SKIPPABLE(60);
	FACE(reimu, unamused);

	// MSG() makes the actor say a line, and then waits an unspecified amount of time (skippable).
	// The timeout is determined by the dialog_util_estimate_wait_timeout_from_text() function in dialog.c
	// MSG() also implies FOCUS()
	MSG(reimu, "Unseasonable snow? I wonder if it’s that ‘Secret God’ again…");

	MSG(cirno, "‘Secret God’?");

	// EVENT()s are handled by stage code.
	// You can find the list of events per dialogue in dialog_interface.h
	// All of them should be signaled eventually.
	EVENT(boss_appears);

	// Wait until the boss slides in.
	// WAIT() can not be skipped.
	WAIT(30);
	WAIT_SKIPPABLE(60);

	// Reveal Cirno’s portrait
	SHOW(cirno);

	MSG(cirno, "What, are you *that* impressed by my invincible magic?!");

	FACE(reimu, sigh);
	MSG(reimu, "Right, of course, I completely forgot about you from before.");
	MSG(reimu, "I should’ve known.");

	// Titles are not yet implemented, but this should work once they are.
	// Right now this does nothing.
	TITLE(cirno, "Cirno", "Thermodynamic Ice Fairy");
	MSG(cirno, "Known better than to intrude on my secret lair!");

	FACE(reimu, normal);
	MSG(reimu, "Well, if you’re excited, I guess something really is going on.");
	MSG(reimu, "I’d ask if you’d just let me through, but…");

	EVENT(music_changes);
	FACE(cirno, angry);
	MSG(cirno, "No way! Prepare to be chilled to the bone!");

	// MSG_UNSKIPPABLE() is like MSG(), but can’t be skipped and takes an explicit timeout.
	//MSG_UNSKIPPABLE(cirno, 300, "You can’t skip this one!");

	// Teardown, must be at the very bottom.
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage1PostBoss) {
	DIALOG_BEGIN(Stage1PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(cirno);

	// VARIANT() changes the base body sprite.
	// Bosses currently only have a "defeated" variant with some clothing damage, to be used in post-battle scenes.
	// Elly’s is called "beaten" and is slightly subdued.
	VARIANT(cirno, defeated);

	// Bosses also have a "defeated" face to go along with the variant, but all the other faces can be used as well.
	// It’s best to set the face to "defeated" in the beginning of a post-battle dialogue, and change it later if needed.
	FACE(cirno, defeated);

	SHOW(cirno);

	FACE(reimu, happy);
	MSG(cirno, "Cool down! I didn’t mean it like that!");
	MSG(reimu, "I did say you could just let me through, you know.");
	MSG(cirno, "I don’t remember that! You broke the rules! I wasn’t ready yet…");

	DIALOG_END();
}

/*
 * Stage 2
 */

DIALOG_TASK(reimu, Stage2PreBoss) {
	DIALOG_BEGIN(Stage2PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(hina);

	HIDE(hina);

	FOCUS(reimu);

	WAIT_SKIPPABLE(60);

	FACE(reimu, unamused);

	MSG(reimu, "Hmm… the mountain is unusually quiet today.");
	MSG(reimu, "With those annoyingly-talkative yōkai gone, it feels practically deserted.");


	EVENT(boss_appears);
	MSG_UNSKIPPABLE(hina, 180, "Ah yes, it’s Ms. Hakurei.");
	SHOW(hina);
	FACE(hina, normal);
	MSG(hina, "I thought you might come.");
	FACE(hina, concerned);
	MSG(hina, "But once again, you’re trying to go places you shouldn’t.");
	MSG(hina, "What a troubled girl…");

	FACE(reimu, unamused);
	MSG(reimu, "So much for ’deserted’…");
	FACE(reimu, normal);
	MSG(reimu, "Do you even know what you’re protecting me from this time?");

	TITLE(hina, "Kagiyama Hina", "Gyroscopic Pestilence God");

	FACE(hina, normal);
	MSG(hina, "You should know that Yōkai Mountain is especially dangeorus today.");
	FACE(hina, concerned);
	MSG(hina, "You ought to turn back now, my dear.");

	FACE(reimu, unamused);
	MSG(reimu, "(She… she completely ignored my question.)");
	MSG(reimu, "Does it matter that the Gods asked me to come this time?");

	EVENT(music_changes);

	MSG(hina, "Dear, I’m certain I know better than the mountain Gods.");
	MSG(hina, "If you do not turn back immediately, I’ll have to repel you by force.");

	FACE(reimu, sigh);
	MSG(reimu, "So annoying…");


	DIALOG_END();
}

DIALOG_TASK(reimu, Stage2PostBoss) {
	DIALOG_BEGIN(Stage2PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(hina);

	WAIT(30);
	VARIANT(hina, defeated);
	FACE(hina, defeated);
	FACE(reimu, normal);

	MSG(hina, "So I lost… you can keep going then, I suppose.");

	MSG(reimu, "You should know by now that I can handle it. Why are you being so overbearing?");

	MSG(hina, "Just don’t say I didn’t warn you, when you inevitably get mauled by whatever’s out there…");

	FACE(reimu, sigh);
	MSG(reimu, "(Ignored again…)");

	DIALOG_END();
}

/*
 * Stage 3
 */

DIALOG_TASK(reimu, Stage3PreBoss) {
	DIALOG_BEGIN(Stage3PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(wriggle);
	HIDE(wriggle);
	FACE(reimu, normal);
	MSG(reimu, "Ugh, what an unsettling feeling. It’s giving me the creeps.");
	MSG(reimu, "But aside from everyone being whipped up into a frenzy, I can’t see anything abnormal yet.");

	EVENT(boss_appears);
	SHOW(wriggle);
	MSG(wriggle, "Well, to your human eyes, insects may seem ’normal’…");
	WAIT(60);
	FACE(wriggle, proud);
	MSG(wriggle, "… but there was a time when we ruled supreme!");

	FACE(reimu, puzzled);
	MSG(reimu, "Eh? You? Aren’t you a bit far from home?");
	MSG(reimu, "Besides, ’ruled supreme’? I’ve never heard of insects ruling over Gensōkyō.");

	TITLE(wriggle, "Wriggle Nightbug", "Insect Rights Activist");

	FACE(wriggle, outraged);
	MSG(wriggle, "Home? But you’ve intruded on my secret lair!");

	FACE(reimu, sigh);
	MSG(reimu, "Again with the ’secret lair’…");

	FACE(wriggle, proud);
	MSG(wriggle, "Haven’t you heard of the Car—…");
	MSG(wriggle, "Carb—…");
	MSG(wriggle, "That period of history?");
	MSG(wriggle, "It was the greatest time, everyone should’ve heard of it!");

	FACE(reimu, normal);
	MSG(reimu, "Seems pretty far-fetched to me, honestly. But you yōkai are always coming up with the craziest theories…");

	FACE(wriggle, outraged);
	MSG(wriggle, "Don’t lump giant insects with common yōkai!");
	FACE(wriggle, proud);
	MSG(wriggle, "I’m going to restore us to our rightful glory of old!");

	FACE(reimu, puzzled);
	MSG(reimu, "Giant insects? But you’re tiny.");
	FACE(reimu, unamused);
	MSG(reimu, "And I still don’t know what you mean by ’glory of old.’");

	EVENT(music_changes);

	MSG(wriggle, "You don’t seem very educated about our history.");
	MSG(wriggle, "So allow me to be your teacher!");
	DIALOG_END();
}

DIALOG_TASK(reimu, Stage3PostBoss) {
	DIALOG_BEGIN(Stage3PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(wriggle);

	VARIANT(wriggle, defeated);
	FACE(wriggle, defeated);

	FACE(reimu, smug);
	MSG(reimu, "Giant insects roaming Gensōkyō, huh?");
	MSG(reimu, "Sounds like one of Sanae’s ’nuclear’ experiments…");

	MSG(wriggle, "B-but I saw it clearly… the fate of insectkind…");

	FACE(reimu, normal);
	MSG(reimu, "Don’t you have a pretty sweet deal as it is, though?");
	MSG(reimu, "You practically own Gensōkyō’s forests.");

	MSG(wriggle, "But can’t we ever hope for more…?");

	DIALOG_END();
}

/*
 * Stage 4
 */

DIALOG_TASK(reimu, Stage4PreBoss) {
	DIALOG_BEGIN(Stage4PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(kurumi);

	HIDE(kurumi);

	FACE(reimu, puzzled);
	MSG(reimu, "Huh? There’s a mansion jutting out of this tower?");
	FACE(reimu, sigh);
	MSG(reimu, "This makes even less sense than usual.");

	EVENT(boss_appears);
	WAIT(30);

	FACE(reimu, normal);
	MSG(reimu, "And who are you supposed to be?");

	SHOW(kurumi);
	FACE(kurumi, tsun);
	MSG(kurumi, "Oi, that’s what I should be asking!");

	FACE(reimu, puzzled);
	MSG(reimu, "…");
	FACE(reimu, normal);
	MSG(reimu, "And what are you doing, anyways?");
	MSG(reimu, "Are you behind all these… strange thoughts everyone’s having?");

	FACE(kurumi, normal);
	TITLE(kurumi, "Kurumi", "High-Society Phlebotomist");
	MSG(kurumi, "I have no idea what you mean by ’strange thoughts,’ but I’ll be honest with you…");
	MSG(kurumi, "Your clothing is giving me some strange thoughts!");
	MSG(kurumi, "Where’d you even get it, the trash?!");

	FACE(reimu, unamused);
	MSG(reimu, "Ugh, another prissy mansion-dweller.");
	FACE(reimu, sigh);
	MSG(reimu, "And to think I thought I remembered you from somewhere. Absolutely not.");
	MSG(reimu, "Just… stop whatever it is you’re doing, at once.");

	FACE(kurumi, dissatisfied);
	MSG(kurumi, "Once you get some fashion sense, maybe I will!");

	FACE(reimu, unamused);
	MSG(reimu, "… so you *are* behind this, huh?");
	FACE(reimu, sigh);
	MSG(reimu, "Haven’t you ever seen a shrine maiden before?");

	FACE(kurumi, normal);
	MSG(kurumi, "You’re a shrine maiden?");
	MSG(kurumi, "More like… BAD maiden!");

	FACE(reimu, unamused);
	MSG(reimu, "T-this is what a shrine maiden wears. It’s a uniform.");

	MSG(kurumi, "Uniform? Are you kidding me?");
	MSG(kurumi, "I’ve seen a real shrine maiden before, and she didn’t look anything like you!");
	MSG(kurumi, "What’s with those patterns, anwyays? It’s completely unorthodox!");

	MSG(reimu, "… ARGH!");
	//FACE(reimu, furious);
	MSG(reimu, "Listen! It’s HARD to find clothes I can wear every day on the job!");
	MSG(reimu, "It has to fit a certain style! And only a few fabrics feel good to wear!");
	FACE(reimu, unamused);
	MSG(reimu, "I can’t stand scratchy wool or slippery silk!");
	MSG(reimu, "Everything else feels terrible again my skin—");
	//FACE(reimu, furious);
	MSG(reimu, "… damnit! Now I’m all worked up!");

	EVENT(music_changes);
	MSG(reimu, "I hate prissy mansion dwellers!");
	MSG(reimu, "Let’s just get this over with!");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage4PostBoss) {
	DIALOG_BEGIN(Stage4PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(kurumi);
	VARIANT(kurumi, defeated);
	FACE(kurumi, defeated);

	FACE(reimu, unamused);
	MSG(reimu, "That’s what you get for insulting my outfit like that.");

	MSG(kurumi, "Ouch ouch ouch…");
	MSG(kurumi, "Wait, what happened?");

	MSG(reimu, "What do you mean ’what happened’?");

	FACE(kurumi, normal);
	MSG(kurumi, "Wait! Aren’t you that girl from a long while back?");

	FACE(reimu, puzzled);
	//FACE(reimu, unsettled);
	MSG(reimu, "I don’t remember you at all? It’s just that unsettling feeling again…");
	MSG(reimu, "So familiar…");
	FACE(reimu, sigh);
	MSG(reimu, "It’s annoying, whatever it is. Make it stop, or tell me how to make it stop.");

	FACE(kurumi, defeated);
	MSG(kurumi, "(Ugh, her clothes really do suck though…)");
	MSG(kurumi, "I was supposed to keep people out until my friend completed their plan…");
	MSG(kurumi, "But then I started feeling weird and kept talking like I had no filter?");
	MSG(kurumi, "It was kinda liberating but also made me really rude?!");

	MSG(reimu, "(Just another victim…?)");
	FACE(reimu, normal);
	MSG(reimu, "Stick around. I want to talk to you after I settle this.");

	DIALOG_END();
}

/*
 * Stage 5
 */

DIALOG_TASK(reimu, Stage5PreBoss) {
	DIALOG_BEGIN(Stage5PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(iku);
	HIDE(iku);

	FACE(reimu, puzzled);
	//FACE(reimu, unsettled);

	MSG(reimu, "I’ve got a bad feeling about this…");
	MSG(reimu, "Or maybe I’m just feeling nauseous with this tower’s constant spinning.");

	EVENT(boss_appears);
	WAIT(60);
	SHOW(iku);
	FACE(iku, normal);
	MSG(iku, "Perchance it a side effect of the tower’s presence?");

	MSG(reimu, "What did you attack me for earlier?");

	TITLE(iku, "Nagae Iku", "Fulminologist of the Heavens");
	MSG(iku, "Ah, apologies. It must have been friendly fire.");
	FACE(iku, eyes_closed);
	MSG(iku, "Things are quite hectic here. I beg your understanding.");
	MSG(iku, "It has been a while, has it not?");

	FACE(reimu, normal);
	MSG(reimu, "Err, right, yes, it has been a while.");

	FACE(iku, serious);
	MSG(iku, "To get straight to the point, this tower is unlike anything I have seen before.");
	MSG(iku, "Since word spread through the clouds, I’ve learned that the culprit is connected to ’parallel universes.’");

	FACE(reimu, puzzled);
	MSG(reimu, "You mean like Otherworlds? That’s not too out of the ordinary these days though.");
	//FACE(reimu, unsettled);
	MSG(reimu, "This feels different though. It’s like the walls are sucking up faith into a void.");
	MSG(reimu, "I’m surprised the fairies are surviving at all…");
	FACE(reimu, normal);
	MSG(reimu, "But what are you doing here? Aren’t you worried about being ’affected’?");

	FACE(iku, eyes_closed);
	MSG(iku, "It is not just the Netherworld and Gensōkyō being affected.");
	FACE(iku, serious);
	MSG(iku, "In Bhava-agra, the Eldest Daughter’s personality has experienced some changes, too.");

	FACE(reimu, surprised);
	MSG(reimu, "It’s spreading that fast?!");

	FACE(iku, smile);
	MSG(iku, "Initially, I had no plans towards interfering. I am not one to take the lead.");
	FACE(iku, serious);
	MSG(iku, "However, the tower’s psychological effects began to intensify, and…");

	FACE(reimu, sigh);
	MSG(reimu, "Do people have that little faith in me…?");

	FACE(iku, eyes_closed);
	MSG(iku, "As more minds began to fall prey, I considered a theory…");
	MSG(iku, "Is this machine incompatible with the existence of fantasy?");
	FACE(iku, normal);
	MSG(iku, "That may be why you feel a sense of dread about the place.");

	FACE(reimu, normal);
	MSG(reimu, "Well, that settles it. Where’s the owner of this place?");

	FACE(iku, smile);
	MSG(iku, "To be frank, I am not sure whether you are up to the task.");
	MSG(iku, "You are still relatively composed, yes, but your mental state seems to be deteriorating.");

	FACE(reimu, unamused);
	MSG(reimu, "I don’t trust anyone else to get this done!");

	EVENT(music_changes);
	MSG(iku, "You are still so worrisome. You ought to go home before you lose yourself.");

	//FACE(reimu, furious);
	MSG(reimu, "No!");

	FACE(iku, serious);
	MSG(iku, "I beg your pardon?");

	FACE(reimu, unamused);
	MSG(reimu, "Don’t keep telling me how little you think of me. I’m sick of it!");
	MSG(reimu, "I’m going to give everyone a reason to have faith in me!");

	MSG(iku, "It seems your resolve is unbreakable. Very well, then.");
	MSG(iku, "Allow me to test your resistance to their infuriating machine!");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage5PostMidBoss) {
	DIALOG_BEGIN(Stage5PostMidBoss);

	ACTOR_LEFT(reimu);
	FACE(reimu, surprised);

	// should be only one message with a fixed 120-frames timeout
	MSG_UNSKIPPABLE(reimu, 120, "The oarfish? Why’s SHE here?");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage5PostBoss) {
	DIALOG_BEGIN(Stage5PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(iku);
	VARIANT(iku, defeated);
	FACE(iku, defeated);
	//FACE(reimu, unsettled);
	FACE(reimu, sigh);

	MSG(iku, "Ah…");
	MSG(iku, "I was wrong for doubting you.");

	MSG(reimu, "Somehow I don’t feel too satisfied by that.");

	FACE(iku, eyes_closed);
	MSG(iku, "To be honest with you, I’m glad you arrived after all.");
	MSG(iku, "I know now that I would not have been able to defeat her.");
	MSG(iku, "Since you’re still able to think clearly, perhaps Gensōkyō will be saved after all.");

	FACE(reimu, normal);

	MSG(reimu, "But you’re right. This is bigger than Gensōkyō.");
	MSG(reimu, "I’ll need to take this seriously.");

	FACE(iku, normal);
	MSG(iku, "Good. Keep climbing until you reach the top floor. You will undoubtedly meet her there.");

	DIALOG_END();
}

/*
 * Stage 6
 */

DIALOG_TASK(reimu, Stage6PreBoss) {
	DIALOG_BEGIN(Stage6PreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(elly);
	HIDE(elly);
	EVENT(boss_appears);
	MSG_UNSKIPPABLE(elly, 180, "Well, well, well. It seems a rat snuck into our tower.");
	SHOW(elly);

	//FACE(elly, smug);
	MSG(elly, "How strange. You should’ve lost your mind by now.");

	//FACE(reimu, unsettled);
	FACE(reimu, unamused);
	MSG(reimu, "I don’t really care why you’re doing this, just turn it off.");

	TITLE(elly, "Elly", "The Theoretical Reaper");
	FACE(elly, angry);
	MSG(elly, "Bold of you to barge into MY tower and give ME orders!");
	MSG(elly, "I still remember what you did to Lady Kazami and I back then!");

	FACE(reimu, sigh);
	MSG(reimu, "And what was that, again?");
	MSG(reimu, "I’m getting a familiar feeling just looking at you, but…");

	FACE(elly, shouting);
	MSG(elly, "Is this a joke?! Are you trying to mock me?!");

	//FACE(reimu, outraged);
	FACE(reimu, unamused);
	MSG(reimu, "I think I would’ve remembered a giant tower that makes people go mad with power!");
	MSG(reimu, "The least you can do when accusing someone is to tell them what they’ve supposedly done!");

	//FACE(elly, smug);
	FACE(elly, normal);
	MSG(elly, "Ah. I see.");
	MSG(elly, "Fine. I will.");
	MSG(elly, "This tower is a recent invention. Its mere existence is revolutionary in several scientific fields.");
	MSG(elly, "The folks of this Gensōkyō ought to have been thankful for its vast scientific knowledge…");
	MSG(elly, "But alas, we had other plans.");

	FACE(reimu, unamused);
	MSG(reimu, "Somehow, it feels like more of a punishment.");
	MSG(reimu, "Ever heard of ’ignorance is bliss’?");
	MSG(reimu, "There won’t be anyone to rule over, or whatever, if you make them all go off.");

	MSG(elly, "The Tower of Babel is capable of ’enlightening’ those fortunate to be around it.");
	MSG(elly, "Anyone unable to comprehend its brilliance will be driven to madness.");
	FACE(elly, shouting);
	MSG(elly, "But the residents of this Gensōkyō mean less than nothing to us! This is simply retribution!");

	//FACE(reimu, unsettled);
	FACE(reimu, normal);
	MSG(reimu, "’This’ Gensōkyō…?");
	MSG(reimu, "’Parallel universes,’ is that it? Like what Iku told me…");

	FACE(elly, normal);
	MSG(elly, "Hmm? Is that what this Gensōkyō is?");

	FACE(reimu, unamused);
	MSG(reimu, "You tell me! You’re the megalomaniac!");
	MSG(reimu, "Do you hate me because some other Reimu did something? Is that it?!");
	MSG(reimu, "But then what does that have to do with any of us?!");
	MSG(reimu, "For a scientist, you’re not making a whole lot of sense!");

	FACE(elly, angry);
	MSG(elly, "Perhaps you’ve simply forgotten, like you have of the rest of us!");
	MSG(elly, "Only the privileged, such as Lady Kazami, made it out! The rest of us were lost!");

	//FACE(reimu, unsettled);
	FACE(reimu, normal);
	MSG(reimu, "You mean Kazami Yūka? And I suppose… Alice, too?");
	MSG(reimu, "I remember when I first met Alice, she made weird comments like you, too.");
	FACE(reimu, unamused);
	MSG(reimu, "But she didn’t go around threatening everyone, did she?!");

	FACE(elly, angry);
	MSG(elly, "We weren’t as lucky! And as reparations, we will take control of this Gensōkyō!");
	MSG(elly, "We will make it so we will NEVER be forgotten!");

	FACE(reimu, sigh);
	MSG(reimu, "Even in Gensōkyō, you can be forgotten, huh?");
	MSG(reimu, "What a miserable thought. No wonder I felt terrible just seeing this place!");
	MSG(reimu, "Turn it off, or being forgotten will be the least of your worries!");

	EVENT(music_changes);
	FACE(elly, shouting);
	MSG(elly, "It’s too late for that! Soon, all of Gensōkyō will become enlightened!");
	MSG(elly, "Don’t you understand, pitiful shrine maiden? Science is an unfathomable power, surpassed by nothing!");
	MSG(elly, "We will subjugate this world at any cost!");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage6PreFinal) {
	DIALOG_BEGIN(Stage6PreFinal);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(elly);
	VARIANT(elly, beaten);
	FACE(elly, shouting);
	FACE(reimu, unamused);
	MSG(reimu, "Is that it? Are you finally done?!");
	MSG(elly, "Y-you think this is over?! The time of reckoning is upon you!");
	MSG(elly, "Gaze upon the ultimate truth of the universe, and tremble!");

	DIALOG_END();
}

/*
 * Extra Stage
 */

#include "stages/extra.h"

DIALOG_TASK(reimu, StageExPreBoss) {
	DIALOG_BEGIN(StageExPreBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(yumemi);
	yumemi.draw_dynamic_overlay = extra_draw_yumemi_portrait_overlay;

	MSG(yumemi, "Oh. The impossible shrine maiden has arrived.");
	EVENT(boss_appears);
	MSG(yumemi, "Here to set me on the right path, just like in your tall tales?");
	FACE(reimu, unamused);
	MSG(reimu, "I get the worst feeling looking at you.");
	FACE(yumemi, surprised);
	MSG(yumemi, "Oh? How’s that?");
	FACE(reimu, sigh);
	MSG(reimu, "It’s mostly just how miserable you look.");
	MSG(yumemi, "Do I look especially miserable? Hmm.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "And what would you know about misery, living in this supposed paradise?");
	FACE(reimu, unamused);
	MSG(yumemi, "Have the other fairytale creatures hurt your feelings?");
	FACE(yumemi, normal);
	FACE(reimu, surprised);
	MSG(reimu, "*Other* fairytale creatures?");
	FACE(reimu, unamused);
	MSG(reimu, "I’m human, you know.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Oh, please. You’re an invincible, flying, magic-wielding shrine maiden—");
	FACE(yumemi, normal);
	FACE(reimu, normal);
	MSG(reimu, "I don’t use magic.");
	FACE(yumemi, smug);
	MSG(yumemi, "Hah.");
	FACE(yumemi, normal);
	MSG(yumemi, "The world - the real world - is dying, shrine maiden.");
	MSG(yumemi, "It doesn’t even matter what ‘solution’ you believe in anymore. Nobody contributes anything to society.");
	FACE(yumemi, sad);
	MSG(yumemi, "People put their hearts and souls into fantasy worlds, leaving nothing left to build the real world on which they depend to live.");
	FACE(reimu, puzzled);
	MSG(reimu, "How’s that Gensōkyō’s problem?");
	MSG(reimu, "It’s not like we invaded you and put you on that path.");
	FACE(yumemi, sigh);
	MSG(yumemi, "If only it were that simple.");
	FACE(yumemi, normal);
	MSG(yumemi, "Places like this tempt people into forgetting their real lives. ");
	FACE(yumemi, sad);
	MSG(yumemi, "And by abandoning the real world, they doom not just themselves, but everyone around them.");
	FACE(reimu, normal);
	MSG(reimu, "People are allowed to have a break from reality.");
	MSG(reimu, "Haven’t you ever read a book?");
	FACE(yumemi, surprised);
	MSG(yumemi, "You mean fiction? I haven’t had a need for it in years.");
	FACE(yumemi, normal);
	MSG(yumemi, "It’s a waste of time.");
	FACE(reimu, surprised);
	MSG(reimu, "Wow, no wonder you’re so miserable.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Regardless, I will end fantasy as we know it.");
	FACE(yumemi, normal);
	FACE(reimu, puzzled);
	MSG(reimu, "What do you mean ‘end’?");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "I’ve given up trying to save my society");
	FACE(yumemi, sad);
	MSG(yumemi, "Even as I introduced this technology to the world, nobody woke up from their dissociative haze. It was as it ever was.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "But at least, with this machine I’ve built, I can erase every so-called ‘Otherworld’ from the fabric of social reality.");
	FACE(yumemi, normal);
	FACE(reimu, assertive);
	MSG(reimu, "People won’t stop day-dreaming just because you tell them to. Fantasy is in people’s hearts and minds.");
	FACE(reimu, sigh);
	MSG(reimu, "So, even if you managed to destroy Gensōkyō—");
	MSG(yumemi, "Youkai, Gods, magicians, flying shrine maidens… it’s all harmful, delusional nonsense.");
	FACE(reimu, unsettled);
	MSG(reimu, "So you want genocide?");
	FACE(yumemi, surprised);
	MSG(yumemi, "Genocide…? What a harsh word. You’re not real. No one is being killed.");
	FACE(reimu, surprised);
	MSG(reimu, "No wonder those two got chills just thinking about you.");
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Ah, I thought I’d made them both understand my way of thinking, by introducing them to the Grand Unified Theory.");
	FACE(yumemi, normal);
	MSG(yumemi, "But in the end, they just betrayed me.");
	FACE(yumemi, sad);
	MSG(yumemi, "It makes sense, though. They can’t betray what they are.");
	FACE(reimu, assertive);
	MSG(reimu, "It’s hard to keep people on your side when all you do is threaten them, idiot!");
	MSG(reimu, "Have you tried not being a genocidal maniac?");
	FACE(yumemi, sigh);
	MSG(yumemi, "I’m tired, shrine maiden. Too tired to explain my life to a figment of imagination.");
	FACE(reimu, unamused);
	MSG(reimu, "Sucks to be you. Don’t make it our problem.");
	EVENT(music_changes);
	FACE(yumemi, normal);
	MSG(yumemi, "Oh, but it’s inherently your problem.");
	FACE(reimu, unsettled);
	FACE(yumemi, eyes_closed);
	MSG(yumemi, "Farewell, shrine maiden. I would say it’s been nice knowing you, but…");
	FACE(yumemi, sad);
	MSG(yumemi, "There’s really no way I could’ve known you at all.");

	DIALOG_END();
}

DIALOG_TASK(reimu, StageExPostBoss) {
	DIALOG_BEGIN(StageExPostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(yumemi);
	yumemi.draw_dynamic_overlay = extra_draw_yumemi_portrait_overlay;

	VARIANT(yumemi, defeated);
	FACE(yumemi, defeated);

	DIALOG_END();
}

/*
 * Register the tasks
 */

#define EXPORT_DIALOG_TASKS_CHARACTER reimu
#include "export_dialog_tasks.inc.h"
