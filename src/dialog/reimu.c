/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "dialog_macros.h"

DIALOG_SCRIPT(reimu_stage1_pre_boss) {
	LEFT("Good grief, it’s too early in the morning to be flying around.");
	RIGHT("Hey, shrine maiden! Am I making you cold?");
	LEFT("Not as much as you’re just being a pest. I’m too busy to play games with fairies.");
	RIGHT("That’s not true! It’s never too early to have fun!");
	RIGHT("Prepare to catch an air conditioner cold on my ice playground!");
}

DIALOG_SCRIPT(reimu_stage1_post_boss) {
	LEFT("A~ah, what a sight! It’s snowing during springtime.");
	LEFT("A little too cold for my taste, though. I'd rather get going.");
}

DIALOG_SCRIPT(reimu_stage2_pre_boss) {
	RIGHT("I can’t let you pass any further than this. Please go back down the mountain.");
	LEFT("You help humans that wander around the mountain, don’t you? So why are you getting in the way?");
	RIGHT("I can feel misfortune leaking from this tunnel. You won’t be going anywhere safe down this route so I need to stop you!");
	LEFT("You should know by now that my job is taking care of dangerous things, so step aside.");
	RIGHT("I’m afraid not!");
}

DIALOG_SCRIPT(reimu_stage2_post_boss) {
	LEFT("In the end, you only made things harder for the both of us.");
	RIGHT("I’m sorry, Miss Shrine Maiden! Would you like a blessing to make up for it?");
	LEFT("No need. I’m the luckiest person in Gensokyo, at least when it’s not about money.");
	LEFT("If you want to help, donate to my shrine. I don’t care if it’s cursed.");
}

DIALOG_SCRIPT(reimu_stage3_pre_boss) {
	LEFT("Huh, a bug that managed to escape extermination.");
	RIGHT("I heard that! ");
	RIGHT("Do you really think I’ll let you get away with such a callous remark?");
	LEFT("You’re just an insect yōkai. What do you think you’re capable of?");
	RIGHT("Why, this entire incident! You’re looking at the culprit right here, right now!");
	LEFT("You can’t be serious. Are you just drunk on all this light?");
	RIGHT("I’m drunk on power! I’ll prove my mettle by defeating you once and for all!");
}

DIALOG_SCRIPT(reimu_stage3_post_boss) {
	LEFT("If humans get drunk from moonshine, then I guess bugs can become intoxicated by sunshine too.");
	RIGHT("Ugh, my head hurts. Can’t you go easy on me?");
	LEFT("I don’t go easy on anyone that claims to be causing an incident. But I can tell you are lying about that.");
	RIGHT("There’s no need to make fun of me for that! You’ll regret it if you lose later!");
	RIGHT("I won’t forget this humiliation!");
	LEFT("Go take a nap already.");
}

DIALOG_SCRIPT(reimu_stage4_pre_boss) {
	RIGHT("Halt, intruder!");
	LEFT("Oh, it’s somebody new.");
	RIGHT("No, definitely not! I could never forget you from all those years ago! That was the most intense battle of my life!");
	LEFT("Hmm, nope, I don’t remember fighting you. Maybe if you told me your name, I could recall faster.");
	RIGHT("Unforgivable! How about I just jog your memory through terror instead?");
	LEFT("I’m in a hurry, and that also sounds unpleasant.");
	RIGHT("You don’t get a choice! Prepare to have bloody nightmares for weeks, shrine maiden!");
}

DIALOG_SCRIPT(reimu_stage4_post_boss) {
	LEFT("See, I don’t scare easily, so that didn’t work. You should have just told me your name.");
	RIGHT("I didn’t think it was possible, but you’re so much stronger than before!");
	RIGHT("Can’t you remember? I’m Kurumi!");
	LEFT("You don’t seem like the sort that is worth remembering after several years.");
	RIGHT("Waah, so mean! You were way cuter back then!");
}

DIALOG_SCRIPT(reimu_stage5_pre_boss) {
	LEFT("I didn’t expect someone actually respectable like you to cause me trouble for no reason.");
	RIGHT("You may be the Hakurei Shrine Maiden, but I’m afraid that you don’t have permission from Heaven to be here.");
	LEFT("What, are you saying that the gods suddenly don’t think I’m fit for duty anymore?");
	RIGHT("No, that’s not it. We already have a more suitable candidate from our concerned Celestials, and she is due to arrive instead.");
	RIGHT("That means you can go home.");
	LEFT("Absolutely not. I’ve worked too hard to get here, so even if I’d like to leave, it’d be far too much wasted time.");
	RIGHT("I’ll compromise by offering a test instead. Defeat me, and you can take our representative’s place.");
	RIGHT("If you can weather the storm I’m about to summon, then I’ll be fully confident that you can strike down the master of this tower!");
}

DIALOG_SCRIPT(reimu_stage5_post_midboss) {
	LEFT("I wonder what’s gotten into that oarfish?");
}

DIALOG_SCRIPT(reimu_stage5_post_boss) {
	LEFT("See? I’ve been solving incidents since the beginning. There is absolutely no one more qualified than me.");
	RIGHT("You’re still as unruly as ever. But you passed, and you certainly hold great power.");
	RIGHT("I’ll let my superiors know that the Hakurei Shrine Maiden arrived to clear up this issue.");
	LEFT("Right, and I’ll make sure it won’t take too long. Tell them to hold tight, and maybe slip me some yen later.");
}

DIALOG_SCRIPT(reimu_stage6_pre_boss) {
	LEFT("Who are you? You’re a lot less intimidating than I expected.");
	RIGHT("I knew you would come here to interrupt me.");
	LEFT("You seem to know who I am, just like that earlier yōkai did. I can’t say the same for myself.");
	RIGHT("That is because we met before in another time, when you were much younger…");
	RIGHT("Back then, I was a mere guard. But then my master left Kurumi and I behind. We fell into the real world instead of Gensōkyō,");
	RIGHT("and I realized that logic was far superior to the chaos of fantasy.");
	LEFT("Do you really think it’s acceptable to threaten Gensōkyō just for some petty delusion of intelligence?");
	RIGHT("An unenlightened fool like you could never see how your faith blinds you.");
	LEFT("You talk big but in the end you’re just a yōkai who is full of herself. It doesn’t matter how smart your world is if it threatens mine.");
	LEFT("I’ll tear everything down all the same.");
	RIGHT("You’ll regret threatening my life’s work when I take your world away. Soon you will be a shrine maiden of nothing!");
}

DIALOG_SCRIPT(reimu_stage6_pre_final) {
	RIGHT("You’ve gotten this far… I can’t believe it! But that will not matter once I show you the truth of this world, and every world.");
	RIGHT("Space, time, dimensions… it all becomes clear when you understand The Theory of Everything!");
	RIGHT("Prepare to see the barrier destroyed!");
}

EXPORT_DIALOG_SCRIPT(reimu)
