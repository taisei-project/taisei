/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "youmu.h"

#define M(side,message) dadd_msg(d,side,message)

static void dialog_youmu_stage1_pre_boss(Dialog *d) {
	M(Left, "The temperature of the lake almost resembles the Netherworld’s. Good thing I don’t get cold easily.");
	M(Right, "What’s that? You think you can’t get cold?");
	M(Left, "I don’t just think that, I know that. I’m half-phantom, so even my body is cold.");
	M(Right, "I’ll take that as a challenge! Prepare to be chilled in a way no ghost can match!");
	M(Right, "Let’s see if phantoms are good as soft-serve ice cream!");
}

static void dialog_youmu_stage1_post_boss(Dialog *d) {
	M(Left, "Lady Yuyuko would probably like trying such an unusual flavor of ice cream. I hope she never gets that idea.");
}

static void dialog_youmu_stage2_pre_boss(Dialog *d) {
	M(Right, "I can’t let you pass any further than this. Please go back down the mountain.");
	M(Left, "Are you a goddess? It’s nice of you to be looking out for me, but the Netherworld has been put at risk due to this incident.");
	M(Left, "I have to keep going.");
	M(Right, "The tunnel leads nowhere but to ruin. If you don’t return to the ghostly world where you come from, I’ll have to stop you by force!");
	M(Left, "My mistress won’t like it if I tell her I was stopped by divine intervention. You’ll have to come up with another excuse.");
}

static void dialog_youmu_stage2_post_boss(Dialog *d) {
	M(Left, "It’s strange, but I feel as if my burdens have been lifted. Did you decide to bless me after all?");
	M(Right, "It’s the least I can do since I cannot stop you. Are you sure you want to go through the tunnel?");
	M(Left, "I don’t have a choice. I’m being ordered to by a power far beyond us both.");
	M(Left, "She’s most definitely stronger than gods when she is angry.");
}

static void dialog_youmu_stage3_pre_boss(Dialog *d) {
	M(Left, "Huh, why is it that I feel like I’ve fought you before? But when I try to recall anything, my skin crawls and I seem to forget immediately.");
	M(Right, "Maybe it’s because you realized the true power of us insects?");
	M(Left, "No, I think it’s because I was too disgusted.");
	M(Right, "How dare you! We make life possible for you humans, and in turn you disrespect us and call us derogatory things!");
	M(Right, "Entering this light has given me great power. I’ll stomp you out in the same cruel way you humans step on bugs!");
	M(Left, "If I cut legs off of an insect, do they squirm all on their own? How gross!");
}

static void dialog_youmu_stage3_post_boss(Dialog *d) {
	M(Right, "I surrender! Please don’t chop anything off!");
	M(Left, "I suppose that as long as you don’t eat anything in my garden, I can let you go.");
	M(Right, "I prefer meat, but not when it’s holding a skewer!");
	M(Right, "Still, I wasn’t able to properly teach you a lesson. You better not lose, for I’ll be happy to strike you when you’re weak!");
	M(Left, "I’m too busy to listen to this buzzing.");
}

static void dialog_youmu_stage4_pre_boss(Dialog *d) {
	M(Right, "Halt, intruder!");
	M(Left, "Oh, and who might you be?");
	M(Right, "Kuru— Hey, my name isn’t important for a nosy person like you to know!");
	M(Left, "I only wanted to know so I could refer to you politely. I don’t want to call you a “random somebody” in your own house.");
	M(Left, "Your house is very nice, by the way. Although I still think Hakugyokurō is bigger. Have you ever been there? Lady Yuyuko loves guests.");
	M(Right, "This isn’t my house, and you’re not allowed to snoop any more than you have! If you keep ignoring me, I’ll have to suck you dry right here where we stand!");
	M(Left, "It’s not your house, and yet you’re telling me to leave? You sound just as presumptuous as the other vampire I’ve met.");
	M(Right, "I can bet you that I’m much more frightening.");
}

static void dialog_youmu_stage4_post_boss(Dialog *d) {
	M(Left, "You’re not as scary as her, or even as good of a host.");
	M(Left, "Maybe you should work on your manners and buy yourself a nice mansion to lord over instead of taking someone else’s.");
	M(Right, "I don’t care about being a stuffy noble! Just leave my friend alone to do her work!");
	M(Left, "Normally I wouldn’t impose on someone who is busy, but I’ll have to make an exception for your mistress this time.");
	M(Right, "…You’re really prim and proper, aren’t you?");
}

static void dialog_youmu_stage5_pre_boss(Dialog *d) {
	M(Left, "You were quite difficult to pin down. Don’t worry; I’ll listen to whatever warning you have before I continue forward.");
	M(Right, "Hmm, you’re the groundskeeper of the Netherworld, correct?");
	M(Left, "That’s right. My mistress sent me here to investigate since the world of spirits has been put in jeopardy by this new world infringing on its boundaries.");
	M(Right, "I’m afraid I cannot let you pass. This new world is a great issue caused by an incredible new power. Only a Celestial or greater is qualified to handle such a dangerous occurrence.");
	M(Left, "That doesn’t seem fair considering I’ve solved incidents before. I know what I am doing, and Lady Yuyuko entrusted me with this.");
	M(Right, "If your confidence will not allow you to back down, then so be it. I will test you using all of Heaven’s might, and if you are unfit, you shall be cast down from this Tower of Babel!");
	M(Left, "I shall pass whatever test necessary if it will allow me to fulfill the wishes of Lady Yuyuko!");
}

static void dialog_youmu_stage5_post_midboss(Dialog *d) {
	// this dialog must contain only one page
	M(Left, "A messenger of Heaven! If I follow her, I’ll surely learn something about the incident!");
}

static void dialog_youmu_stage5_post_boss(Dialog *d) {
	M(Right, "You cut through the cloudbank, and now the storm has cleared. It is unconventional, but I trust you to end the incident.");
	M(Left, "It’s a lot on my shoulders, but I must believe in my capacity for the sake of the Netherworld.");
	M(Left, "As for you, if the Celestials give you any trouble, I’m certain my lady could speak to them on your behalf.");
	M(Right, "Thank you. Now hurry, for the barrier is becoming thinner than ever!");
}	

static void dialog_youmu_stage6_pre_boss(Dialog *d) {
	M(Left, "Are you the one behind this new world?");
	M(Right, "That is correct.");
	M(Left, "Considering that scythe of yours, I’m really quite surprised that a shikigami would be behind everything.");
	M(Right, "That is because you completely misunderstand. I am not a shikigami, and never was. My duty was once to protect a powerful yōkai, but when she left for Gensōkyō, I was left behind.");
	M(Right, "Together with my friend, Kurumi, we left for the Outside World and were stranded there. But I gained an incredible new power once I realized my true calling as a theoretical physicist.");
	M(Left, "I don’t completely follow, but if you’re a yōkai, I can’t see how you could be a scientist. It goes against your nature.");
	M(Right, "An unenlightened fool such as yourself could never comprehend my transcendence.");
	M(Right, "I have become more than a simple yōkai, for I alone am now capable of discovering the secrets behind the illusion of reality!");
	M(Left, "Well, whatever you’re doing here is disintegrating the barrier of common sense and madness keeping Gensōkyō from collapsing into the Outside World.");
	M(Left, "You’re also infringing on the Netherworld’s space and my mistress is particularly upset about that. That means I have to stop you, no matter what.");
	M(Right, "Pitiful servant of the dead. You’ll never be able to stop my life’s work from being fulfilled! I’ll simply unravel that nonsense behind your half-and-half existence!");
}

static void dialog_youmu_stage6_pre_final(Dialog *d) {
	M(Right, "You’ve gotten this far… I can’t believe it! But that will not matter once I show you the truth of this world, and every world.");
	M(Right, "Space, time, dimensions… it all becomes clear when you understand The Theory of Everything!");
	M(Right, "Prepare to see the barrier destroyed!");
}

PlayerDialogProcs dialog_youmu = {
	.stage1_pre_boss = dialog_youmu_stage1_pre_boss,
	.stage1_post_boss = dialog_youmu_stage1_post_boss,
	.stage2_pre_boss = dialog_youmu_stage2_pre_boss,
	.stage2_post_boss = dialog_youmu_stage2_post_boss,
	.stage3_pre_boss = dialog_youmu_stage3_pre_boss,
	.stage3_post_boss = dialog_youmu_stage3_post_boss,
	.stage4_pre_boss = dialog_youmu_stage4_pre_boss,
	.stage4_post_boss = dialog_youmu_stage4_post_boss,
	.stage5_post_midboss = dialog_youmu_stage5_post_midboss,
	.stage5_pre_boss = dialog_youmu_stage5_pre_boss,
	.stage5_post_boss = dialog_youmu_stage5_post_boss,
	.stage6_pre_boss = dialog_youmu_stage6_pre_boss,
	.stage6_pre_final = dialog_youmu_stage6_pre_final,
};
