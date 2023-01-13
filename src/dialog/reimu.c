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

	// Let’s wait a bit, then change Reimu’s expression.
	// All timings are in frames (60 = 1 sec).
	WAIT_SKIPPABLE(60);
	// "normal" is the default face.
	FACE(reimu, normal);

	// MSG() makes the actor say a line, and then waits an unspecified amount of time (skippable).
	// The timeout is determined by the dialog_util_estimate_wait_timeout_from_text() function in dialog.c
	MSG(reimu, "Unseasonable snow? I wonder if it’s that ‘Secret God’ again…");

	// EVENT()s are handled by stage code.
	// You can find the list of events per dialogue in dialog_interface.h
	// All of them should be signaled eventually.
	EVENT(boss_appears);

	// MSG_UNSKIPPABLE() is like MSG(), but can’t be skipped and takes an explicit timeout.
	// Wait until the boss slides in.
	MSG_UNSKIPPABLE(cirno, 180, "Did you just call me a ‘God’?");

	// Reveal Cirno’s portrait
	SHOW(cirno);
	MSG(cirno, "What, are you *that* impressed by my incredible magic?!");

	FACE(reimu, sigh);
	MSG(reimu, "Right, of course, I completely forgot about you from before.");
	MSG(reimu, "Should’ve known.");

	// Titles are not yet implemented, but this should work once they are.
	// Right now this does nothing.
	TITLE(cirno, "Cirno", "Thermodynamic Ice Fairy");
	MSG(cirno, "Known better than to intrude on my secret lair!");

	FACE(reimu, normal);
	MSG(reimu, "Well, if *you’re* excited, I guess something really is going on.");
	MSG(reimu, "I’d ask if you’d just let me through, but…");

	EVENT(music_changes);
	FACE(cirno, angry);
	MSG(cirno, "No way! Prepare to be chilled to the bone!");

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
	MSG(cirno, "Cool down! I didn’t mean it like that!");
	FACE(reimu, smug);
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
	WAIT_SKIPPABLE(60);

	FACE(reimu, normal);
	MSG(reimu, "With those loudmouthed kappa gone, the mountain feels practically deserted.");
	FACE(reimu, sigh);
	MSG(reimu, "It’s almost like I miss them.");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(hina, 180, "Ah, it’s Ms. Hakurei.");
	SHOW(hina);
	FACE(reimu, normal);
	FACE(hina, normal);
	MSG(hina, "I thought you might come.");
	FACE(hina, concerned);
	MSG(hina, "But once again, you’re trying to go places you shouldn’t.");
	MSG(hina, "What a troubled girl…");

	FACE(reimu, sigh);
	MSG(reimu, "So much for ‘deserted’…");
	FACE(reimu, normal);
	MSG(reimu, "Do you even know what you’re protecting me from this time?");

	TITLE(hina, "Kagiyama Hina", "Gyroscopic Pestilence God");

	FACE(hina, normal);
	MSG(hina, "A bright girl like you should know that Yōkai Mountain is especially dangerous today.");
	FACE(hina, concerned);
	MSG(hina, "You ought to turn back now, my dear.");

	FACE(reimu, sigh);
	MSG(reimu, "(She… she completely ignored my question.)");
	FACE(reimu, puzzled);
	MSG(reimu, "Does it matter that the Gods asked me to come this time?");

	EVENT(music_changes);

	MSG(hina, "In this instance, I’m sure I know better than the mountain Gods.");
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
	FACE(reimu, puzzled);
	MSG(reimu, "You should know by now that I can handle it. Why are you being so overbearing?");
	MSG(hina, "Just don’t say I didn’t warn you, when you inevitably get mauled by whatever’s out there…");

	FACE(reimu, unamused);
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
	MSG(reimu, "Aside from everyone being whipped up into a frenzy, I can’t see anything abnormal yet.");

	EVENT(boss_appears);
	MSG_UNSKIPPABLE(wriggle, 180, "Well, to your human eyes, insects may seem ‘normal’…");

	SHOW(wriggle);
	FACE(wriggle, proud);
	MSG(wriggle, "… but there was a time when we ruled supreme!");

	FACE(reimu, puzzled);
	MSG(reimu, "Eh? You? Aren’t you a bit far from home?");
	MSG(reimu, "Besides, ‘ruled supreme’? I’ve never heard of insects ruling over Gensōkyō.");

	TITLE(wriggle, "Wriggle Nightbug", "Insect Rights Activist");
	FACE(wriggle, outraged);
	MSG(wriggle, "Home? But you’ve intruded on my secret lair!");

	FACE(reimu, sigh);
	MSG(reimu, "Again with the ‘secret lair’…");

	FACE(wriggle, proud);
	MSG(wriggle, "Haven’t you heard of the Car—…");
	MSG(wriggle, "Carb—…");
	FACE(reimu, puzzled);
	MSG(wriggle, "That period of history?");
	MSG(wriggle, "It was the greatest time, everyone should’ve heard of it by now!");

	MSG(reimu, "Seems pretty far-fetched to me, honestly. But you yōkai are always coming up with the craziest theories…");

	FACE(wriggle, outraged);
	MSG(wriggle, "Don’t lump us great insects in with common yōkai!");
	FACE(wriggle, proud);
	MSG(wriggle, "I’m going to restore us to our rightful glory of old!");

	FACE(reimu, puzzled);
	MSG(reimu, "Giant insects? But you’re tiny.");
	FACE(reimu, unamused);
	MSG(reimu, "And I still don’t know what you mean by ‘glory of old.’");

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
	MSG(reimu, "Sounds like one of Sanae’s ‘nuclear’ experiments.");

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
	MSG(reimu, "…?");
	FACE(reimu, unamused);
	MSG(reimu, "And what are you doing, anyways?");
	MSG(reimu, "Are you behind all these… strange thoughts everyone’s having?");

	FACE(kurumi, normal);
	TITLE(kurumi, "Kurumi", "High-Society Phlebotomist");
	MSG(kurumi, "I have no idea what you mean by ‘strange thoughts’, but I’ll be honest with you…");
	MSG(kurumi, "Your clothing is giving me some strange thoughts!");
	MSG(kurumi, "Where’d you even get that outfit?! Dumpster diving?! For real!");

	FACE(reimu, unamused);
	MSG(reimu, "Ugh, another prissy mansion-dweller.");
	FACE(reimu, sigh);
	MSG(reimu, "I thought I remembered you from somewhere, but no. Absolutely not.");
	FACE(reimu, assertive);
	MSG(reimu, "Just… stop whatever it is you’re doing, right now.");

	FACE(kurumi, dissatisfied);
	MSG(kurumi, "Once you get some fashion sense, maybe I will!");

	FACE(reimu, unamused);
	MSG(reimu, "… so you *are* behind this, huh?");
	FACE(reimu, sigh);
	MSG(reimu, "Besides, haven’t you ever seen a shrine maiden before?");

	FACE(kurumi, normal);
	MSG(kurumi, "You’re a shrine maiden?");
	FACE(reimu, unamused);
	MSG(kurumi, "More like… BAD maiden!");

	FACE(reimu, assertive);
	MSG(reimu, "T-this is what a shrine maiden wears. It’s a uniform.");

	MSG(kurumi, "Uniform? Are you kidding me?");
	MSG(kurumi, "I’ve seen a real shrine maiden before, and she didn’t look anything like you!");
	MSG(kurumi, "What’s with those patterns, anyways? It’s completely unorthodox!");
	MSG(kurumi, "It’s not even using the right type of fabric! Get out of here, fake-maiden!");

	FACE(reimu, irritated);
	MSG(reimu, "… ARGH!");
	FACE(reimu, outraged);
	MSG(reimu, "Listen!! It’s HARD to find clothes I can wear every day on the job!");
	MSG(reimu, "It has to fit a certain style!");
	MSG(reimu, "And only a few fabrics feel good to wear!");
	MSG(reimu, "I can’t stand scratchy wool or slippery silk!");
	MSG(reimu, "Everything else feels terrible again my skin—");
	FACE(reimu, irritated);
	MSG(reimu, "… damnit! Now I’m all worked up!");

	EVENT(music_changes);

	MSG(reimu, "I hate prissy mansion dwellers!");
	FACE(reimu, outraged);
	MSG(reimu, "Let’s just get this over with!");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage4PostBoss) {
	DIALOG_BEGIN(Stage4PostBoss);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(kurumi);
	VARIANT(kurumi, defeated);
	FACE(kurumi, defeated);

	FACE(reimu, irritated);
	MSG(reimu, "That’s what you get for insulting my outfit like that.");

	MSG(kurumi, "Ouch ouch ouch…");
	MSG(kurumi, "W-wait, what happened?");

	MSG(reimu, "What do you mean ‘what happened’?");

	MSG(kurumi, "(M-maybe if I fake having amnesia… yeah, that’s it…!)");
	FACE(kurumi, normal);
	MSG(kurumi, "O-oof, my head! Ow! How did I even get here???");
	MSG(kurumi, "W-wait! Aren’t you that girl from a long while back?");

	FACE(reimu, unsettled);
	MSG(reimu, "I don’t remember you at all? It’s just that unsettling feeling again…");
	MSG(reimu, "So familiar…");
	FACE(reimu, sigh);
	MSG(reimu, "It’s annoying, whatever it is. Make it stop, or tell me how to make it stop.");

	FACE(kurumi, defeated);
	MSG(kurumi, "Huh? I-I don’t know! I barely remember anything!");
	MSG(kurumi, "(Ugh, her clothes really do suck though…)");

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

	FACE(reimu, unsettled);
	MSG(reimu, "I’ve got a bad feeling about this…");
	FACE(reimu, puzzled);
	MSG(reimu, "Or maybe I’m just feeling nauseous with this tower’s constant spinning.");

	EVENT(boss_appears);
	WAIT(60);
	SHOW(iku);
	FACE(iku, normal);
	MSG(iku, "Or perchance it a side effect of the tower’s mere presence?");

	MSG(reimu, "Oh, it’s you. Why did you attack me earlier, anyway?");

	TITLE(iku, "Nagae Iku", "Fulminologist of the Heavens");
	FACE(iku, smile);
	MSG(iku, "Ah, apologies. It must have been friendly fire.");
	FACE(iku, eyes_closed);
	MSG(iku, "Things are quite hectic here. I beg your understanding.");
	MSG(iku, "But it has been a while, has it not?");

	FACE(reimu, normal);
	MSG(reimu, "Err, right, yes, it has been a while. Nice to see you again.");

	FACE(iku, serious);
	MSG(iku, "To get straight to the point, this tower is unlike anything I have seen before.");
	MSG(iku, "Since word spread through the clouds, I’ve learned that the culprit is connected to ‘parallel universes.’");

	FACE(reimu, puzzled);
	MSG(reimu, "You mean like Otherworlds? That’s not too out of the ordinary these days.");
	FACE(reimu, unsettled);
	MSG(reimu, "This feels different though. It’s like the walls are sucking up faith into a void.");
	MSG(reimu, "I’m surprised the fairies are surviving at all…");
	FACE(reimu, normal);
	MSG(reimu, "But what are you even doing here?");

	FACE(iku, eyes_closed);
	MSG(iku, "It is not just the Netherworld and Gensōkyō being affected.");
	FACE(iku, serious);
	MSG(iku, "In Bhava-agra, the Eldest Daughter’s personality has experienced some changes, too.");

	FACE(reimu, surprised);
	MSG(reimu, "It’s spreading that fast?!");

	FACE(iku, smile);
	MSG(iku, "Initially, I had no plans towards interfering.");
	FACE(reimu, normal);
	FACE(iku, serious);
	MSG(iku, "However, the tower’s psychological effects began to intensify, and I began to fear that nobody would take charge.");

	FACE(reimu, sigh);
	MSG(reimu, "(Do people have this little faith in me…?)");

	FACE(iku, eyes_closed);
	MSG(iku, "As more minds began to fall prey, I considered a theory…");
	FACE(reimu, normal);
	MSG(iku, "Is this machine incompatible with the existence of fantasy?");
	FACE(iku, normal);
	MSG(iku, "That may be why you feel a sense of dread about the place.");

	FACE(reimu, normal);
	MSG(reimu, "Well, that settles it. Where’s the owner, then?");

	FACE(iku, smile);
	MSG(iku, "To be frank, I am not sure whether you are up to the task.");
	MSG(iku, "You are still relatively composed, yes, but your mental state seems to be deteriorating.");

	FACE(reimu, irritated);
	MSG(reimu, "Deteriorating? But I just got here!");

	EVENT(music_changes);
	MSG(iku, "You are still so worrisome. You ought to go home before you lose yourself.");

	FACE(reimu, irritated);
	MSG(reimu, "No!");

	FACE(iku, serious);
	MSG(iku, "I beg your pardon?");

	MSG(reimu, "Don’t keep telling me how little you think of me. I’m sick of it!");
	FACE(reimu, assertive);
	MSG(reimu, "I’m going to give everyone a reason to have faith in me!");

	MSG(iku, "It seems your resolve is unwavering. Very well, then.");
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

	MSG(iku, "Ah…");
	MSG(iku, "I was wrong for doubting you, I suppose.");

	FACE(reimu, unamused);
	MSG(reimu, "Somehow I don’t feel too satisfied by that.");

	FACE(iku, eyes_closed);
	MSG(iku, "To be honest with you, I’m glad you arrived after all.");
	MSG(iku, "I know now that I would not have been able to defeat her.");
	FACE(reimu, normal);
	MSG(iku, "Since you’re still able to think clearly, perhaps Gensōkyō will be saved after all.");

	FACE(reimu, sigh);
	MSG(reimu, "Finally.");
	FACE(reimu, normal);
	MSG(reimu, "But you’re right. This is bigger than Gensōkyō.");
	MSG(reimu, "I’ll need to take this seriously.");

	FACE(iku, normal);
	MSG(iku, "Good.");
	MSG(iku, "Keep climbing until you reach the top floor. You will undoubtedly meet the instigator there.");

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
	MSG_UNSKIPPABLE(elly, 180, "Well, well, well. It seems a rat trespassed on our domain.");
	SHOW(elly);

	FACE(elly, smug);
	MSG(elly, "How strange. You should’ve lost your mind by now.");

	FACE(reimu, unsettled);
	MSG(reimu, "Just turn it off already.");
	MSG(reimu, "I’m not in the mood to argue, so don’t push me!");

	TITLE(elly, "Elly", "The Theoretical Reaper");
	FACE(elly, angry);
	MSG(elly, "Bold of you to barge into MY tower and give ME orders!");
	MSG(elly, "I still remember what you did to Lady Kazami and I back then!");

	FACE(reimu, sigh);
	MSG(reimu, "And what was that, again?");
	MSG(reimu, "I’m getting a familiar feeling just looking at you, but…");

	FACE(elly, shouting);
	MSG(elly, "Is this a joke?! Are you trying to mock me?!");

	FACE(reimu, assertive);
	MSG(reimu, "I think I would’ve remembered a giant tower that makes people go mad with power!");
	MSG(reimu, "The least you can do when accusing someone is to tell them what they’ve supposedly done!");

	FACE(elly, smug);
	MSG(elly, "Ah. I see.");
	MSG(elly, "Fine. I will.");
	FACE(elly, normal);
	FACE(reimu, unamused);
	MSG(elly, "This tower is a recent invention. Its mere existence is revolutionary in several scientific fields.");
	MSG(elly, "The folks of this Gensōkyō ought to have been thankful for its vast scientific knowledge…");
	MSG(elly, "But alas, we have other plans.");

	FACE(reimu, unamused);
	MSG(reimu, "Somehow, it feels like more of a punishment.");
	MSG(reimu, "Ever heard of ‘ignorance is bliss’?");
	MSG(reimu, "There won’t be anyone to rule over, or whatever, if you make them all lose it.");

	MSG(elly, "The Tower of Babel is capable of ‘enlightening’ those fortunate to be around it.");
	FACE(elly, smug);
	MSG(elly, "Anyone unable to comprehend its brilliance will be driven to madness.");
	FACE(elly, shouting);
	MSG(elly, "But the residents of this Gensōkyō mean less than nothing to us! This is simply retribution!");

	FACE(reimu, unsettled);
	MSG(reimu, "‘This’ Gensōkyō…?");
	MSG(reimu, "‘Parallel universes’, is that it?");

	FACE(elly, normal);
	MSG(elly, "Hmm? Is that what you think this Gensōkyō is?");

	FACE(reimu, irritated);
	MSG(reimu, "You tell me! You’re the megalomaniac!");
	FACE(reimu, assertive);
	MSG(reimu, "Do you hate me because some other Reimu did something? Is that it?");
	MSG(reimu, "But then what does that have to do with any of us?!");
	FACE(reimu, irritated);
	MSG(reimu, "For a ‘scientist’, you’re not making a whole lot of sense!");

	FACE(elly, angry);
	MSG(elly, "Perhaps you’ve simply forgotten, like you have of the rest of us!");
	FACE(reimu, unamused);
	MSG(elly, "Only the privileged, such as Lady Kazami, made it out! The rest of us were lost!");

	FACE(reimu, unsettled);
	MSG(reimu, "You mean Kazami Yūka? And I suppose… Alice, too?");
	MSG(reimu, "I remember when I first met Alice, and she said weird things too, just like you are now.");
	FACE(reimu, unamused);
	MSG(reimu, "But she didn’t go around threatening everyone!");

	FACE(elly, angry);
	MSG(elly, "We weren’t as lucky! And as reparations, we will take control of this Gensōkyō!");
	MSG(elly, "We will make it so we will NEVER be forgotten!");

	FACE(reimu, sigh);
	MSG(reimu, "Even in Gensōkyō, you can be forgotten, huh?");
	MSG(reimu, "What a miserable thought. No wonder I felt terrible just seeing this place!");
	FACE(reimu, assertive);
	MSG(reimu, "Turn it off, or being forgotten will be the least of your worries!");

	EVENT(music_changes);
	FACE(elly, shouting);
	MSG(elly, "It’s too late for that! Soon, all of Gensōkyō will become enlightened!");
	FACE(reimu, unsettled);
	MSG(elly, "Don’t you understand, pitiful shrine maiden? Science is an unfathomable power, surpassed by nothing!");
	MSG(elly, "We won’t be forgotten by this world, at any cost!");

	DIALOG_END();
}

DIALOG_TASK(reimu, Stage6PreFinal) {
	DIALOG_BEGIN(Stage6PreFinal);

	ACTOR_LEFT(reimu);
	ACTOR_RIGHT(elly);
	VARIANT(elly, beaten);
	FACE(elly, shouting);
	FACE(reimu, assertive);
	MSG(reimu, "Is that it? Are you finally done?");
	MSG(elly, "Y-you think this is over?! The time of reckoning is upon you!");
	MSG(elly, "Gaze upon the ultimate truth of the universe, and tremble!");

	DIALOG_END();
}

/*
 * Register the tasks
 */

#define EXPORT_DIALOG_TASKS_CHARACTER reimu
#include "export_dialog_tasks.inc.h"
