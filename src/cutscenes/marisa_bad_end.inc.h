
.name = "Marisa (Bad Ending)",
.bgm = "ending",

.phases = (CutscenePhase[]) {
	{ "cutscenes/locations/sdm", {
		T_NARRATOR("— The Scarlet Devil Mansion\n"),
		T_NARRATOR("A peculiar western mansion in an eastern wonderland.\n"),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/01", {
		T_NARRATOR("A nervous witch sat flipping through endless stacks of books, drinking tea instead of her usual sake…\n"),
		T_MARISA("Hey Patchy, do ya got any more books on magitech?"),
		T_PATCHOULI("Yes, but I doubt they’ll be of much use. That machine is beyond that subject entirely."),
		T_MARISA("Ugh, you’re probably right, as usual."),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/01", {
		T_NARRATOR("Marisa closed the book and let it slam onto the floor."),
		T_PATCHOULI("Mind your manners! That’s a very old tome!"),
		T_MARISA("It’s so irritatin’! I thought I solved everything but now that Tower’s just sittin’ there, ‘n the whole Mountain’s been quarantined…!"),
		T_MARISA("It just ain’t satisfyin’, y’know?"),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/01", {
		T_FLANDRE("I could destroy it! Blow it sky high!"),
		T_MARISA("Gah! … now what did I say about sneakin’ up on people, Flan?"),
		T_FLANDRE("Sorry!"),
		T_PATCHOULI("Thank you for the offer, Flandre, but I think it’s already ‘sky high’."),
		T_FLANDRE("Awww… how boring…"),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/02", {
		T_NARRATOR("After having tea with the pair of them, Marisa wandered the stacks, looking for inspiration.\n"),
		T_NARRATOR("Suddenly, she felt as if she'd found a loose floorboard, tripping and falling flat on her face.\n"),
		T_NARRATOR("When she looked back to see what she'd tripped on, she didn’t see anything except the immaculate and expertly-organized rows of bookshelves.\n"),
		T_NARRATOR("Then, she noticed something cold and thin underneath her hand."),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/02", {
		T_NARRATOR("After some time, she realized it was a ‘Smart Device’ of some kind, like a handheld computer.\n"),
		T_NARRATOR("This one was far more advanced than any she'd seen from the Outside World, including that young occultist’s phone.\n"),
		T_NARRATOR("It behaved both as a solid and a liquid, able to rapidly change shapes by applying pressure to certain sides, into different form factors: a phone, a book, a digital typewriter with a keyboard…\n"),
		T_NARRATOR("Once Marisa worked out how to make it display something, the title of a book appeared on its dim screen…"),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/02", {
		T_NARRATOR("Advanced & Practical Computational Applications of the Grand Unified Theory\n\n"),
		T_NARRATOR("— by Usami Renko"),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/02", {
		T_CENTERED("— Bad End —", "Try to reach the end without using a Continue!"),
		{ 0 },
	}},
	{ NULL }
}
