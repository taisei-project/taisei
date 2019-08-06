/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "dialog_macros.h"

#define LEFT_BASE youmu

#define RIGHT_BASE cirno

DIALOG_SCRIPT(youmu_stage1_pre_boss) {
	LEFT_FACE(relaxed);
	LEFT("The temperature of the lake almost resembles the Netherworld’s. Good thing I don’t get cold easily.");
	LEFT_FACE(surprised);
	RIGHT("What’s that? You think you can’t get cold?");
	LEFT_FACE(unamused);
	LEFT("I don’t just think that, I know that. I’m half-phantom, so even my body is cold.");
	RIGHT_FACE(angry);
	RIGHT("I’ll take that as a challenge! Prepare to be chilled in a way no ghost can match!");
	LEFT_FACE(normal);
	RIGHT("Let’s see if phantoms are good as soft-serve ice cream!");
}

DIALOG_SCRIPT(youmu_stage1_post_boss) {
	LEFT_FACE(happy);
	LEFT("Lady Yuyuko would probably like trying such an unusual flavor of ice cream.");
	LEFT_FACE(normal);
	LEFT("…I hope she never gets the idea.");
}

#undef RIGHT_BASE
#define RIGHT_BASE hina

DIALOG_SCRIPT(youmu_stage2_pre_boss) {
	RIGHT("I can’t let you pass any further than this. Please go back down the mountain.");
	LEFT_FACE(surprised);
	LEFT("Are you a goddess? It’s nice of you to be looking out for me, but the Netherworld has been put at risk due to this incident.");
	LEFT_FACE(normal);
	RIGHT_FACE(concerned);
	LEFT("I have to keep going.");
	RIGHT("The tunnel leads nowhere but to ruin. If you don’t return to the ghostly world where you come from, I’ll have to stop you by force!");
	LEFT_FACE(sigh);
	LEFT("My mistress won’t like it if I tell her I was stopped by divine intervention. You’ll have to come up with another excuse.");
	LEFT_FACE(normal);
	RIGHT_FACE(serious);
	RIGHT("No I won't!");
}

DIALOG_SCRIPT(youmu_stage2_post_boss) {
	LEFT_FACE(happy);
	LEFT("It’s strange, but I feel as if my burdens have been lifted. Did you decide to bless me after all?");
	LEFT_FACE(relaxed);
	RIGHT("It’s the least I can do since I cannot stop you. Are you sure you want to go through the tunnel?");
	LEFT_FACE(normal);
	LEFT("I don’t have a choice. I’m being ordered to by a power far beyond us both.");
	LEFT("She’s most definitely stronger than gods when she is angry.");
}

#undef RIGHT_BASE
#define RIGHT_BASE wriggle

DIALOG_SCRIPT(youmu_stage3_pre_boss) {
	LEFT_FACE(puzzled);
	LEFT("Huh, why is it that I feel like I’ve fought you before? But when I try to recall anything, my skin crawls and I seem to forget immediately.");
	RIGHT_FACE(proud);
	LEFT_FACE(normal);
	RIGHT("Maybe it’s because you realized the true power of us insects?");
	LEFT_FACE(eeeeh);
	LEFT("No, I think it’s because I was too disgusted.");
	LEFT_FACE(normal);
	RIGHT_FACE(outraged);
	RIGHT("How dare you! We make life possible for you humans, and in turn you disrespect us and call us derogatory things!");
	RIGHT_FACE(calm);
	LEFT_FACE(relaxed);
	RIGHT("Entering this light has given me great power. I’ll stomp you out in the same cruel way you humans step on bugs!");
	LEFT_FACE(smug);
	LEFT("If I cut legs off of an insect, do they squirm all on their own? How gross!");
}

DIALOG_SCRIPT(youmu_stage3_post_boss) {
	LEFT_FACE(relaxed);
	RIGHT("I surrender! Please don’t chop anything off!");
	LEFT_FACE(happy);
	LEFT("I suppose that as long as you don’t eat anything in my garden, I can let you go.");
	RIGHT("I prefer meat, but not when it’s holding a skewer!");
	LEFT_FACE(relaxed);
	RIGHT_FACE(proud);
	RIGHT("Still, I wasn’t able to properly teach you a lesson. You better not lose, for I’ll be happy to strike you when you’re weak!");
	LEFT_FACE(unamused);
	LEFT("I’m too busy to listen to this buzzing.");
}

#undef RIGHT_BASE
#define RIGHT_BASE kurumi

DIALOG_SCRIPT(youmu_stage4_pre_boss) {
	LEFT_FACE(surprised);
	RIGHT_FACE(tsun);
	RIGHT("Halt, intruder!");
	RIGHT_FACE(normal);
	LEFT_FACE(puzzled);
	LEFT("Oh, and who might you be?");
	RIGHT("I'm Kuru—");
	RIGHT_FACE(tsun_blush);
	RIGHT("Hey, my name isn’t important for a nosy person like you to know!");
	LEFT_FACE(unamused);
	LEFT("I only wanted to know so I could refer to you politely. I don’t want to call you a “random somebody” in your own house.");
	RIGHT_FACE(normal);
	LEFT_FACE(relaxed);
	LEFT("Your house is very nice, by the way. Although I still think Hakugyokurō is bigger. Have you ever been there? Lady Yuyuko loves guests.");
	RIGHT_FACE(tsun);
	RIGHT("This isn’t my house, and you’re not allowed to snoop any more than you have! If you keep ignoring me, I’ll have to suck you dry right here where we stand!");
	LEFT_FACE(sigh);
	LEFT("It’s not your house, and yet you’re telling me to leave? You sound just as presumptuous as the other vampire I’ve met.");
	LEFT_FACE(normal);
	RIGHT("I can bet you that I’m much more frightening.");
}

DIALOG_SCRIPT(youmu_stage4_post_boss) {
	LEFT_FACE(smug);
	LEFT("You’re not as scary as her, or even as good of a host.");
	LEFT_FACE(relaxed);
	LEFT("Maybe you should work on your manners and buy yourself a nice mansion to lord over instead of taking someone else’s.");
	RIGHT_FACE(tsun_blush);
	RIGHT("I don’t care about being a stuffy noble! Just leave my friend alone to do her work!");
	LEFT_FACE(happy);
	LEFT("Normally I wouldn’t impose on someone who is busy, but I’ll have to make an exception for your mistress this time.");
	LEFT_FACE(relaxed);
	RIGHT_FACE(defeated);
	RIGHT("…You’re really prim and proper, aren’t you?");
}

#undef RIGHT_BASE
#define RIGHT_BASE iku

DIALOG_SCRIPT(youmu_stage5_pre_boss) {
	LEFT("You were quite difficult to pin down. Don’t worry; I’ll listen to whatever warning you have before I continue forward.");
	RIGHT("Hmm, you’re the groundskeeper of the Netherworld, correct?");
	LEFT("That’s right. My mistress sent me here to investigate since the world of spirits has been put in jeopardy by this new world infringing on its boundaries.");
	RIGHT("I’m afraid I cannot let you pass. This new world is a great issue caused by an incredible new power. Only a Celestial or greater is qualified to handle such a dangerous occurrence.");
	LEFT_FACE(eeeeh);
	LEFT("That doesn’t seem fair considering I’ve solved incidents before. I know what I am doing, and Lady Yuyuko entrusted me with this.");
	LEFT_FACE(unamused);
	RIGHT("If your confidence will not allow you to back down, then so be it. I will test you using all of Heaven’s might, and if you are unfit, you shall be cast down from this Tower of Babel!");
	LEFT_FACE(sigh);
	LEFT("I shall pass whatever test necessary if it will allow me to fulfill the wishes of Lady Yuyuko!");
}

DIALOG_SCRIPT(youmu_stage5_post_midboss) {
	// this dialog must contain only one page
	LEFT_FACE(surprised);
	LEFT("A messenger of Heaven! If I follow her, I’ll surely learn something about the incident!");
}

DIALOG_SCRIPT(youmu_stage5_post_boss) {
	RIGHT("You cut through the cloudbank, and now the storm has cleared. It is unconventional, but I trust you to end the incident.");
	LEFT_FACE(relaxed);
	LEFT("It’s a lot on my shoulders, but I must believe in my capacity for the sake of the Netherworld.");
	LEFT("As for you, if the Celestials give you any trouble, I’m certain my lady could speak to them on your behalf.");
	LEFT_FACE(normal);
	RIGHT("Thank you. Now hurry, for the barrier is becoming thinner than ever!");
}

#undef RIGHT_BASE
#define RIGHT_BASE elly

DIALOG_SCRIPT(youmu_stage6_pre_boss) {
	LEFT("Are you the one behind this new world?");
	RIGHT("That is correct.");
	LEFT("Considering that scythe of yours, I’m really quite surprised that a shikigami would be behind everything.");
	LEFT_FACE(surprised);
	RIGHT("That is because you completely misunderstand. I am not a shikigami, and never was. My duty was once to protect a powerful yōkai, but when she left for Gensōkyō, I was left behind.");
	LEFT_FACE(puzzled);
	RIGHT("Together with my friend, Kurumi, we left for the Outside World and were stranded there. But I gained an incredible new power once I realized my true calling as a theoretical physicist.");
	LEFT("I don’t completely follow, but if you’re a yōkai, I can’t see how you could be a scientist. It goes against your nature.");
	RIGHT("An unenlightened fool such as yourself could never comprehend my transcendence.");
	RIGHT("I have become more than a simple yōkai, for I alone am now capable of discovering the secrets behind the illusion of reality!");
	LEFT_FACE(sigh);
	LEFT("Well, whatever you’re doing here is disintegrating the barrier of common sense and madness keeping Gensōkyō from collapsing into the Outside World.");
	LEFT_FACE(normal);
	LEFT("You’re also infringing on the Netherworld’s space and my mistress is particularly upset about that. That means I have to stop you, no matter what.");
	RIGHT_FACE(angry);
	RIGHT("Pitiful servant of the dead. You’ll never be able to stop my life’s work from being fulfilled!");
	LEFT_FACE(surprised);
	RIGHT("I’ll simply unravel that nonsense behind your half-and-half existence!");
}

DIALOG_SCRIPT(youmu_stage6_pre_final) {
	RIGHT_FACE(angry);
	RIGHT("You’ve gotten this far… I can’t believe it! But that will not matter once I show you the truth of this world, and every world.");
	LEFT_FACE(puzzled);
	RIGHT("Space, time, dimensions… it all becomes clear when you understand The Theory of Everything!");
	LEFT_FACE(surprised);
	RIGHT("Prepare to see the barrier destroyed!");
}

EXPORT_DIALOG_SCRIPT(youmu)
