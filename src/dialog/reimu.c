/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "reimu.h"

#define M(side,message) dadd_msg(d,side,message)

static void dialog_reimu_stage1_pre_boss(Dialog *d) {
	M(Left, "Good grief, it’s too early in the morning to be flying around.");
	M(Right, "Hey, shrine maiden! Am I making you cold?");
	M(Left, "Not as much as you’re just being a pest. I’m too busy to play games with fairies.");
	M(Right, "That’s not true! It’s never too early to have fun!");
	M(Right, "Prepare to catch an air conditioner cold on my ice playground!");
}

static void dialog_reimu_stage1_post_boss(Dialog *d) {
	M(Left, "A~ah, what a sight! It’s snowing during springtime.");
	M(Left, "A little too cold for my taste, though. I'd rather get going.");
}

static void dialog_reimu_stage2_pre_boss(Dialog *d) {
	M(Right, "I can’t let you pass any further than this. Please go back down the mountain.");
	M(Left, "You help humans that wander around the mountain, don’t you? So why are you getting in the way?");
	M(Right, "I can feel misfortune leaking from this tunnel. You won’t be going anywhere safe down this route so I need to stop you!");
	M(Left, "You should know by now that my job is taking care of dangerous things, so step aside.");
	M(Right, "I’m afraid not!");
}

static void dialog_reimu_stage2_post_boss(Dialog *d) {
	M(Left, "In the end, you only made things harder for the both of us.");
	M(Right, "I’m sorry, Miss Shrine Maiden! Would you like a blessing to make up for it?");
	M(Left, "No need. I’m the luckiest person in Gensokyo, at least when it’s not about money.");
	M(Left, "If you want to help, donate to my shrine. I don’t care if it’s cursed.");
}

static void dialog_reimu_stage3_pre_boss(Dialog *d) {
	M(Left, "Huh, a bug that managed to escape extermination.");
	M(Right, "I heard that! ");
	M(Right, "Do you really think I’ll let you get away with such a callous remark?");
	M(Left, "You’re just an insect yōkai. What do you think you’re capable of?");
	M(Right, "Why, this entire incident! You’re looking at the culprit right here, right now!");
	M(Left, "You can’t be serious. Are you just drunk on all this light?");
	M(Right, "I’m drunk on power! I’ll prove my mettle by defeating you once and for all!");
}

static void dialog_reimu_stage3_post_boss(Dialog *d) {
	M(Left, "If humans get drunk from moonshine, then I guess bugs can become intoxicated by sunshine too.");
	M(Right, "Ugh, my head hurts. Can’t you go easy on me?");
	M(Left, "I don’t go easy on anyone that claims to be causing an incident. But I can tell you are lying about that.");
	M(Right, "There’s no need to make fun of me for that! You’ll regret it if you lose later!");
	M(Right, "I won’t forget this humiliation!");
	M(Left, "Go take a nap already.");
}

static void dialog_reimu_stage4_pre_boss(Dialog *d) {
	M(Right, "Halt, intruder!");
	M(Left, "Oh, it’s somebody new.");
	M(Right, "No, definitely not! I could never forget you from all those years ago! That was the most intense battle of my life!");
	M(Left, "Hmm, nope, I don’t remember fighting you. Maybe if you told me your name, I could recall faster.");
	M(Right, "Unforgivable! How about I just jog your memory through terror instead?");
	M(Left, "I’m in a hurry, and that also sounds unpleasant.");
	M(Right, "You don’t get a choice! Prepare to have bloody nightmares for weeks, shrine maiden!");
}

static void dialog_reimu_stage4_post_boss(Dialog *d) {
	M(Left, "See, I don’t scare easily, so that didn’t work. You should have just told me your name.");
	M(Right, "I didn’t think it was possible, but you’re so much stronger than before!");
	M(Right, "Can’t you remember? I’m Kurumi!");
	M(Left, "You don’t seem like the sort that is worth remembering after several years.");
	M(Right, "Waah, so mean! You were way cuter back then!");
}

static void dialog_reimu_stage5_pre_boss(Dialog *d) {
	M(Left, "I didn’t expect someone actually respectable like you to cause me trouble for no reason.");
	M(Right, "You may be the Hakurei Shrine Maiden, but I’m afraid that you don’t have permission from Heaven to be here.");
	M(Left, "What, are you saying that the gods suddenly don’t think I’m fit for duty anymore?");
	M(Right, "No, that’s not it. We already have a more suitable candidate from our concerned Celestials, and she is due to arrive instead.");
	M(Right, "That means you can go home.");
	M(Left, "Absolutely not. I’ve worked too hard to get here, so even if I’d like to leave, it’d be far too much wasted time.");
	M(Right, "I’ll compromise by offering a test instead. Defeat me, and you can take our representative’s place.");
	M(Right, "If you can weather the storm I’m about to summon, then I’ll be fully confident that you can strike down the master of this tower!");
}

static void dialog_reimu_stage5_post_midboss(Dialog *d) {
	M(Left, "I wonder what’s gotten into that oarfish?");
}

static void dialog_reimu_stage5_post_boss(Dialog *d) {
	M(Left, "See? I’ve been solving incidents since the beginning. There is absolutely no one more qualified than me.");
	M(Right, "You’re still as unruly as ever. But you passed, and you certainly hold great power.");
	M(Right, "I’ll let my superiors know that the Hakurei Shrine Maiden arrived to clear up this issue.");
	M(Left, "Right, and I’ll make sure it won’t take too long. Tell them to hold tight, and maybe slip me some yen later.");
}

static void dialog_reimu_stage6_pre_boss(Dialog *d) {
	M(Left, "Who are you? You’re a lot less intimidating than I expected.");
	M(Right, "I knew you would come here to interrupt me.");
	M(Left, "You seem to know who I am, just like that earlier yōkai did. I can’t say the same for myself.");
	M(Right, "That is because we met before in another time, when you were much younger…");
	M(Right, "Back then, I was a mere guard. But then my master left Kurumi and I behind. We fell into the real world instead of Gensōkyō,");
	M(Right, "and I realized that logic was far superior to the chaos of fantasy.");
	M(Left, "Do you really think it’s acceptable to threaten Gensōkyō just for some petty delusion of intelligence?");
	M(Right, "An unenlightened fool like you could never see how your faith blinds you.");
	M(Left, "You talk big but in the end you’re just a yōkai who is full of herself. It doesn’t matter how smart your world is if it threatens mine.");
	M(Left, "I’ll tear everything down all the same.");
	M(Right, "You’ll regret threatening my life’s work when I take your world away. Soon you will be a shrine maiden of nothing!");
}

static void dialog_reimu_stage6_pre_final(Dialog *d) {
	M(Right, "You’ve gotten this far… I can’t believe it! But that will not matter once I show you the truth of this world, and every world.");
	M(Right, "Space, time, dimensions… it all becomes clear when you understand The Theory of Everything!");
	M(Right, "Prepare to see the barrier destroyed!");
}

PlayerDialogProcs dialog_reimu = {
	.stage1_pre_boss = dialog_reimu_stage1_pre_boss,
	.stage1_post_boss = dialog_reimu_stage1_post_boss,
	.stage2_pre_boss = dialog_reimu_stage2_pre_boss,
	.stage2_post_boss = dialog_reimu_stage2_post_boss,
	.stage3_pre_boss = dialog_reimu_stage3_pre_boss,
	.stage3_post_boss = dialog_reimu_stage3_post_boss,
	.stage4_pre_boss = dialog_reimu_stage4_pre_boss,
	.stage4_post_boss = dialog_reimu_stage4_post_boss,
	.stage5_post_midboss = dialog_reimu_stage5_post_midboss,
	.stage5_pre_boss = dialog_reimu_stage5_pre_boss,
	.stage5_post_boss = dialog_reimu_stage5_post_boss,
	.stage6_pre_boss = dialog_reimu_stage6_pre_boss,
	.stage6_pre_final = dialog_reimu_stage6_pre_final,
};
