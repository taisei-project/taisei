/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "dialog_macros.h"

#define LEFT_BASE marisa

#define RIGHT_BASE cirno

DIALOG_SCRIPT(marisa_stage1_pre_boss) {
	LEFT("It’s gotten pretty cold ’round here. I wish I brought a sweater.");
	RIGHT("Every time there’s an incident, we fairies show up to stop weak humans like you from spoiling the fun!");
	LEFT("So, you’re callin’ me weak?");
	RIGHT("Weak to cold for sure! I’ll turn you into a human-sized popsicle!");
	LEFT_FACE(smug);
	LEFT("I’d like to see ya try.");
}

DIALOG_SCRIPT(marisa_stage1_post_boss) {
	RIGHT("Waah, it’s hot! I’m gonna melt!");
	LEFT_FACE(happy);
	LEFT("I’ve made the lake a lot warmer now, so ya can’t freeze anyone.");
}

#undef RIGHT_BASE
#define RIGHT_BASE hina

DIALOG_SCRIPT(marisa_stage2_pre_boss) {
	RIGHT("I can’t let you pass any further than this. Please go back down the mountain.");
	LEFT_FACE(smug);
	LEFT("So, is that your misfortune talkin’?");
	RIGHT_FACE(concerned);
	RIGHT("Exploring that tunnel is only going to lead you to ruin. I’ll protect you by driving you away!");
	LEFT_FACE(unamused);
	LEFT("I can make dumb decisions on my own, thanks.");
	RIGHT_FACE(serious);
	LEFT_FACE(puzzled);
	RIGHT("A bad attitude like that is destined to be cursed from the beginning. I’ll send you packing home as a favor to you, so you don’t get hurt further by your terrible ideas!");
}

DIALOG_SCRIPT(marisa_stage2_post_boss) {
	LEFT_FACE(smug);
	LEFT("Somehow I already feel luckier after beating ya. Fixin’ the border should be no sweat!");
	LEFT_FACE(normal);
	RIGHT("It’s as much as I can do since I cannot stop you. But maybe you could reconsider…?");
	LEFT_FACE(happy);
	LEFT("Nope! I’m way too excited!");
}

#undef RIGHT_BASE
#define RIGHT_BASE wriggle

DIALOG_SCRIPT(marisa_stage3_pre_boss) {
	LEFT_FACE(puzzled);
	LEFT("Huh, didn’t expect a bug to make it this far down the tunnel.");
	RIGHT("All of this beautiful light, and you didn’t expect me to be drawn to it?");
	LEFT_FACE(smug);
	LEFT("I expected ya to lose, that’s all.");
	RIGHT_FACE(proud);
	RIGHT("I’m nowhere near as weak as you think, human! In fact, being here has made me stronger than ever! That’s because I’m the one that made this tunnel and broke the barrier,");
	LEFT_FACE(normal);
	RIGHT_FACE(calm);
	RIGHT("in order to finally show you all the true merit of insect power!");
	LEFT_FACE(smug);
	LEFT("I can’t really say I believe ya on that point, bein’ that you’re just a bug ’n all.");
	RIGHT_FACE(proud);
	RIGHT("Ha! Watch me put on a grand light show that will put your dud fireworks to shame!");
}

DIALOG_SCRIPT(marisa_stage3_post_boss) {
	LEFT_FACE(smug);
	LEFT("So where’s the finale? Nothing’s changed.");
	RIGHT("I was lying! I just wanted to take you down a peg with my new magic!");
	LEFT_FACE(normal);
	LEFT("Y’know, fly too close to the sun and ya get burned. I’ll be goin’ to take care of some real threats now.");
	RIGHT("Ugh, just wait and see! This won’t be over if I ever get the chance again!");
	LEFT_FACE(happy);
	LEFT("Yeah, yeah. You’ll need to be less crispy first, though.");
}

#undef RIGHT_BASE
#define RIGHT_BASE kurumi

DIALOG_SCRIPT(marisa_stage4_pre_boss) {
	LEFT_FACE(surprised);
	RIGHT("Halt, intruder!");
	LEFT_FACE(normal);
	LEFT("Now here’s a face I haven’t seen in a long time. What’s this fancy house all about anyways? Decided to guard Yūka again, or is it somebody new?");
	RIGHT("It’s none of your business, that’s what it is. My friend is busy with some important work and she can’t be disturbed by nosy humans poking into where they don’t belong.");
	LEFT_FACE(happy);
	LEFT("Now that’s no way to treat an old acquaintance! I know ya from before, so how about just givin’ me a backstage pass, eh?");
	RIGHT("If you know me as well as you think you do, you’d know that all you’re going to get acquainted with is the taste of your own blood!");
	LEFT_FACE(unamused);
	LEFT("Shoot, so that’s a no to my reservation, right?");
}

DIALOG_SCRIPT(marisa_stage4_post_boss) {
	RIGHT("Oh no, I lost! She’ll be really angry if someone interferes with her preparations!");
	LEFT_FACE(smug);
	LEFT("Right, got any tickets left then? I’ll probably be a little early, but I wanna meet the star of this show.");
	LEFT("She seems like somebody I’d know.");
	RIGHT("Attendance might as well be free since I’m too drained to do anything about it.");
	LEFT_FACE(happy);
	LEFT("Now that’s what I like t’ hear!");
}

#undef RIGHT_BASE
#define RIGHT_BASE iku

DIALOG_SCRIPT(marisa_stage5_pre_boss) {
	LEFT("I finally caught up to ya! I should’ve known that the Dragon’s messenger would be hard to chase down in the air.");
	LEFT_FACE(puzzled);
	LEFT("Are you part of the incident too?");
	RIGHT("Weren’t those earlier bombs enough of a deterrent? As this world is cutting into the space of Heaven, only those authorized are allowed to investigate.");
	RIGHT("You’re not a Celestial or anyone else from Heaven. That means you cannot go further.");
	LEFT_FACE(happy);
	LEFT("C’mon, you’re good at readin’ the atmosphere, right? Then ya should know that I’m not gonna back down after comin’ this far.");
	LEFT_FACE(normal);
	RIGHT("I don’t have time to reason with you, unfortunately.");
	RIGHT("When it comes to an average human sticking out arrogantly, there’s only one reasonable course of action for a bolt of lightning to take.");
	LEFT_FACE(surprised);
	RIGHT("Prepare to be struck down from Heaven’s door!");
}

DIALOG_SCRIPT(marisa_stage5_post_midboss) {
	// this dialog must contain only one page
	LEFT("Hey, wait! …Did I just see an oarfish?");
}

DIALOG_SCRIPT(marisa_stage5_post_boss) {
	RIGHT("You weathered my assault well. It is unconventional, but I’ll let you take care of this.");
	LEFT_FACE(happy);
	LEFT("Ha, I’m always fixin’ problems back home. I’ve got all the right credentials!");
	LEFT_FACE(normal);
	RIGHT("…I’m going to get into so much trouble, aren’t I?");
	LEFT_FACE(smug);
	LEFT("If you want, I can put in a good word for ya in the bureaucracy. Ask me later, all right?");
	RIGHT("As much as I appreciate the offer, I’d rather not have you create a bigger mess.");
	LEFT_FACE(happy);
	LEFT("No guarantees!");
}

#undef RIGHT_BASE
#define RIGHT_BASE elly

DIALOG_SCRIPT(marisa_stage6_pre_boss) {
	LEFT("And so the big bad boss behind it all is…");
	LEFT_FACE(surprised);
	LEFT("Wait, Elly?!");
	RIGHT("That’s strange; I thought Kurumi was keeping people from sneaking into my research facility.");
	LEFT("You have a lab?! Last time we fought, ya were just some random guard. You didn’t even know any science!");
	RIGHT("After Yūka was defeated and left us for Gensōkyō, Kurumi and I decided we needed to do some soul searching and find another job.");
	LEFT_FACE(puzzled);
	RIGHT("We stranded ourselves in the Outside World on accident, and I found my true calling as a theoretical physicist.");
	RIGHT("This world I’ve built… this is a world dictated not by the madness of Gensōkyō, but one of reason, logic, and common sense!");
	LEFT_FACE(normal);
	LEFT("Yeah, and that’s why it broke the barrier, didn’t it? You can’t have a borin’ world like this next t’ Gensōkyō without causin’ us issues.");
	RIGHT_FACE(angry);
	RIGHT("An unenlightened fool like you wouldn’t understand… You’re even still using magic, of all things!");
	LEFT_FACE(smug);
	LEFT("I understand enough to know that you’re the problem. And like any good mathematician, I’m gonna solve it by blastin’ it to bits!");
	RIGHT("No! You’ll never destroy my life’s work! I’ll dissect that nonsensical magic of yours, and then I’ll tear it apart!");
}

DIALOG_SCRIPT(marisa_stage6_pre_final) {
	RIGHT_FACE(angry);
	RIGHT("You’ve gotten this far… I can’t believe it! But that will not matter once I show you the truth of this world, and every world.");
	LEFT_FACE(puzzled);
	RIGHT("Space, time, dimensions… it all becomes clear when you understand The Theory of Everything!");
	LEFT_FACE(surprised);
	RIGHT("Prepare to see the barrier destroyed!");
}

EXPORT_DIALOG_SCRIPT(marisa)
