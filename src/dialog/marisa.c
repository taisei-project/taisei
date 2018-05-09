/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "marisa.h"

#define M(side,message) dadd_msg(d,side,message)

void dialog_marisa_stage1(Dialog *d) {
	M(Left, "It’s gotten pretty cold ’round here. I wish I brought a sweater.");
	M(Right, "Every time there’s an incident, we fairies show up to stop weak humans like you from spoiling the fun!");
	M(Left, "So, you’re callin’ me weak?");
	M(Right, "Weak to cold for sure! I’ll turn you into a human-sized popsicle!");
	M(Left, "I’d like to see ya try.");
}

void dialog_marisa_stage1_post(Dialog *d) {
	M(Right, "Waah, it's hot!  I'm gonna melt!");
	M(Left, "I’ve made the lake a lot warmer now, so ya can’t freeze anyone.");
}

void dialog_marisa_stage2(Dialog *d) {
	M(Right, "I can’t let you pass any further than this. Please go back down the mountain.");
	M(Left, "So, is that your misfortune talkin’?");
	M(Right, "Exploring that tunnel is only going to lead you to ruin. I’ll protect you by driving you away!");
	M(Left, "I can make dumb decisions on my own, thanks.");
	M(Right, "A bad attitude like that is destined to be cursed from the beginning. I’ll send you packing home as a favor to you, so you don’t get hurt further by your terrible ideas!");
}

void dialog_marisa_stage2_post(Dialog *d) {
	M(Left, "Somehow I already feel luckier after beating ya. Fixin’ the border should be no sweat!");
	M(Right, "It's as much as I can do since I cannot stop you.  But maybe you could reconsider…?");
	M(Left, "Nope!  I'm way too excited!");
}

void dialog_marisa_stage3(Dialog *d) {
	M(Left, "Huh, didn’t expect a bug to make it this far down the tunnel.");
	M(Right, "All of this beautiful light, and you didn’t expect me to be drawn to it?");
	M(Left, "I expected ya to lose, that’s all.");
	M(Right, "I’m nowhere near as weak as you think, human! In fact, being here has made me stronger than ever! That’s because I’m the one that made this tunnel and broke the barrier,");
	M(Right, "in order to finally show you all the true merit of insect power!");
	M(Left, "I can’t really say I believe ya on that point, bein’ that you’re just a bug ’n all.");
	M(Right, "Ha! Watch me put on a grand light show that will put your dud fireworks to shame!");
}

void dialog_marisa_stage3_post(Dialog *d) {
	M(Left, "So where's the finale?  Nothing's changed.");
	M(Right, "I was lying!  I just wanted to take you down a peg with my new magic!");
	M(Left, "Y'know, fly too close to the sun and ya get burned.  I'll be goin' to take care of some real threats now.");
	M(Right, "Ugh, just wait and see!  I'll win anyway as long as you lose!");
}

void dialog_marisa_stage4(Dialog *d) {
	M(Right, "Halt, intruder!");
	M(Left, "Now here’s a face I haven’t seen in a long time. What’s this fancy house all about anyways? Decided to guard Yūka again, or is it somebody new?");
	M(Right, "It’s none of your business, that’s what it is. My friend is busy with some important work and she can’t be disturbed by nosy humans poking into where they don’t belong.");
	M(Left, "Now that’s no way to treat an old acquaintance! I know ya from before, so how about just givin’ me a backstage pass, eh?");
	M(Right, "If you know me as well as you think you do, you’d know that all you’re going to get acquainted with is the taste of your own blood!");
	M(Left, "Shoot, so that’s a no to my reservation, right?");
}

void dialog_marisa_stage4_post(Dialog *d) {
	M(Right, "Oh no, I lost!  She'll be really angry if someone interferes with her preparations!");
	M(Left, "Right, got any tickets left then?  I'll probably be a little early, but I wanna meet the star of this show.");
	M(Left, "She seems like somebody I'd know.");
	M(Right, "Attendance might as well be free since I'm too drained to do anything about it.");
	M(Left, "Now that's what I like t' hear!");
}

void dialog_marisa_stage5(Dialog *d) {
	M(Left, "I finally caught up to ya! I should’ve known that the Dragon’s messenger would be hard to chase down in the air.");
	M(Left, "Are you part of the incident too?");
	M(Right, "Weren’t those earlier bombs enough of a deterrent? As this world is cutting into the space of Heaven, only those authorized are allowed to investigate.");
	M(Right, "You’re not a Celestial or anyone else from Heaven. That means you cannot go further.");
	M(Left, "C’mon, you’re good at readin’ the atmosphere, right? Then ya should know that I’m not gonna back down after comin’ this far.");
	M(Right, "I don’t have time to reason with you, unfortunately.");
	M(Right, "When it comes to an average human sticking out arrogantly, there’s only one reasonable course of action for a bolt of lightning to take.");
	M(Right, "Prepare to be struck down from Heaven’s door!");
}

void dialog_marisa_stage5_mid(Dialog *d) {
	// this dialog must contain only one page
	M(Left, "Hey, wait! …Did I just see an oarfish?");

}

void dialog_marisa_stage5_post(Dialog *d) {
	M(Right, "You weathered my assault well.  It is unconventional, but I'll let you take care of this.");
	M(Left, "Ha, I'm always fixin' problems back home.  I've got all the right credentials!");
	M(Right, "…I'm going to get into so much trouble, aren't I?");
	M(Left, "If you want, I can put in a good word for ya in the bureaucracy.  Ask me later, all right?");
	M(Right, "As much as I appreciate the offer, I'd rather not have you create a bigger mess.");
	M(Left, "No guarantees!");
}

void dialog_marisa_stage6(Dialog *d) {
	M(Left, "And so the big bad boss behind it all is…");
	M(Left, "Wait, Elly?!");
	M(Right, "That’s strange; I thought Kurumi was keeping people from sneaking into my research facility.");
	M(Left, "You have a lab?! Last time we fought, ya were just some random guard. You didn’t even know any science!");
	M(Right, "After Yūka was defeated and left us for Gensōkyō, Kurumi and I decided we needed to do some soul searching and find another job.");
	M(Right, "We stranded ourselves in the Outside World on accident, and I found my true calling as a theoretical physicist.");
	M(Right, "This world I’ve built… this is a world dictated not by the madness of Gensōkyō, but one of reason, logic, and common sense!");
	M(Left, "Yeah, and that’s why it broke the barrier, didn’t it? You can’t have a borin’ world like this next t’ Gensōkyō without causin’ us issues.");
	M(Right, "An unenlightened fool like you wouldn’t understand… You’re even still using magic, of all things!");
	M(Left, "I understand enough to know that you’re the problem. And like any good mathematician, I’m gonna solve it by blastin’ it to bits!");
	M(Right, "No! You’ll never destroy my life’s work! I’ll dissect that nonsensical magic of yours, and then I’ll tear it apart!");

}

void dialog_marisa_stage6_inter(Dialog *d) {
	M(Right, "You’ve gotten this far… I can’t believe it! But that will not matter once I show you the truth of this world, and every world.");
	M(Right, "Space, time, dimensions… it all becomes clear when you understand The Theory of Everything!");
	M(Right, "Prepare to see the barrier destroyed!");
}
