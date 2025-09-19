
.name = N_("Yōmu (Bad Ending)"),
.bgm = "ending",

.phases = (CutscenePhase[]) {
	{ "cutscenes/locations/tower", {
		T_NARRATOR("— The Tower of Babel"),
		T_NARRATOR("The infernal machine from another world."),
		{ 0 },
	}},
	{ "cutscenes/locations/tower", {
		T_NARRATOR("At the foot of the Tower that now imposed itself on Gensōkyō’s skyline…"),
		T_NARRATOR("…the greatest swordswoman of Gensōkyō and the Netherworld, Konpaku Yōmu, stood over a makeshift grave, praying.\n"),
		T_NARRATOR("No, she hadn’t murdered her opponent. It was, perhaps, a little more troubling than that…"),
		{ 0 },
	}},
	{ "cutscenes/youmu_bad/01", {
		T_YUYUKO("Tell me again… she simply vanished? In mid-air?"),
		T_YOUMU("Yes. One moment she was there, and the next, she was not."),
		T_YOUMU("The vampire girl from before also disappeared, in addition to all of the loitering fairies."),
		T_YOUMU("It was as if everything within it ceased to exist entirely, except myself."),
		{ 0 },
	}},
	{ "cutscenes/youmu_bad/01", {
		T_YOUMU("Thus, I must assumed I have caused their demise in some way, despite my best intentions."),
		T_YOUMU("That woman, Elly… we knew each other for such a short time, yet her passion and fury affected me so deeply."),
		T_YUYUKO("Hmm… I don’t sense any lost souls here, and nobody liked that passed through the Netherworld."),
		T_YUYUKO("I doubt she has truly died, then… ah, but all this ‘parallel universes’ stuff is beyond me, hehehe."),
		{ 0 },
	}},
	{ "cutscenes/locations/tower", {
		T_NARRATOR("The Tower had ceased functioning, returning the land and yōkai around it to some semblance of status quo.\n"),
		T_NARRATOR("But the instigators were nowhere to be found. It was as if they had never existed.\n"),
		T_NARRATOR("Yōmu's newfound fearlessness had been tempered as well, replaced with a sense of loss."),
		{ 0 },
	}},
	{ "cutscenes/youmu_bad/02", {
		T_YOUMU("Thank you for humouring your humble servant’s odd request, Lady Yuyuko."),
		T_YUYUKO("Oh my, think nothing of it! You did great!"),
		T_YUYUKO("But since we’re in the neighbourhood… do you think the kappa are running one of their famous festival grill stands?"),
		T_YUYUKO("As a celebration for the incident being resolved, perhaps…?"),
		T_YUYUKO("We could go on a fun date! And I’m feeling a bit peckish…"),
		{ 0 },
	}},
	{ "cutscenes/youmu_bad/02", {
		T_NARRATOR("The kappa of Yōkai Mountain were throwing a ‘Gaze Upon The Incredible Tower (From A Safe Distance)’ festival, to encourage human and yōkai tourism after everyone had been scared off by the Tower.\n"),
		T_NARRATOR("Although, Yōmu found it difficult to enjoy herself, of course.\n"),
		T_NARRATOR("Somewhere, somehow, she hoped the furiously righteous scythe-bearer was still out there, waiting for their rematch…"),
		{ 0 },
	}},
	{ "cutscenes/youmu_bad/02", {
		T_CENTERED(N_("— Bad End —"), N_("Try to reach the end without using a Continue!")),
		{ 0 },
	}},
	{ NULL }
}
