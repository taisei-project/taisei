
.name = "Reimu (Bad Ending)",
.bgm = "ending",

.phases = (CutscenePhase[]) {
	{ "cutscenes/locations/hakurei", {
		T_NARRATOR("— The Hakurei Shrine\n"),
		T_NARRATOR("The shrine at the border of fantasy and reality.\n"),
		T_NARRATOR("The Tower had powered down, yet the instigators were nowhere to be found.\n"),
		T_NARRATOR("Reimu returned to her shrine, unsure if she had done anything at all.\n"),
		T_NARRATOR("The Gods and yōkai of the Mountain shuffled about the shrine quietly, with bated breath.\n"),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/01", {
		T_NARRATOR("Reimu sighed, and made her announcement.\n"),
		T_REIMU("Listen up! I stopped the spread of madness, but until further notice, Yōkai Mountain is off limits!"),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/01", {
		T_NARRATOR("Nobody was pleased with that.\n"),
		T_NARRATOR("The Moriya Gods were visibly shocked.\n"),
		T_NARRATOR("The kappa demanded that they be able to fetch their equipment, and tend to their hydroponic cucumber farms.\n"),
		T_NARRATOR("The tengu furiously scribbled down notes, once again as if they’d had the scoop of the century, before also beginning to make their own demands.\n"),
		{ 0 },
	}},
	{ "cutscenes/locations/hakurei", {
		T_NARRATOR("Once Reimu had managed to placate the crowd, she sat in the back of the Hakurei Shrine, bottle of sake in hand.\n"),
		T_NARRATOR("She didn’t feel like drinking, however. She nursed it without even uncorking it, sighing to herself."),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/02", {
		T_YUKARI("Oh my, a little dissatisfied, aren’t we?"),
		T_NARRATOR("\nReimu was never surprised by Yukari’s sudden appearances anymore, of course.\n"),
		T_YUKARI("It’s unlike you to stop until everything is business as usual, Reimu."),
		T_YUKARI("Depressed?"),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/02", {
		T_REIMU("Yeah. That place sucked."),
		T_YUKARI("Perhaps I should’ve gone with you, hmm?"),
		T_REIMU("Huh? Why? Do you wanna gap the whole thing out of Gensōkyō?"),
		T_YUKARI("That might be a bit much, even for me, dear."),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/02", {
		T_REIMU("Whatever that ‘madness’ thing was, it was getting to me, too. It made me feel frustrated and… lonely?"),
		T_REIMU("But why would I feel lonely? It doesn’t make any sense."),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/02", {
		T_YUKARI("So, what will you do?"),
		T_REIMU("Find a way to get rid of it, eventually. I’m sure there’s an answer somewhere out there…"),
		T_NARRATOR("\nYukari smiled.\n"),
		T_YUKARI("Glad to hear it. It’s your duty, after all."),
		{ 0 },
	}},
	{ "cutscenes/reimu_bad/02", {
		T_CENTERED("— Bad End —", "Try to reach the end without using a Continue!"),
		{ 0 },
	}},
	{ NULL }
}
