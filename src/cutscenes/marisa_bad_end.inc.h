
.name = "Marisa (Bad Ending)",
.bgm = "ending",

.phases = (CutscenePhase[]) {
	{ "cutscenes/locations/sdm", {
		T_NARRATOR("— The Scarlet Devil Mansion"),
		T_NARRATOR("A peculiar western mansion in an eastern wonderland."),
		T_NARRATOR("A nervous witch sat flipping through endless stacks of books, drinking tea instead of her usual sake…"),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/01", {
		T_MARISA("Hey Patchy, do ya got any more books on Magitech?"),
		T_PATCHOULI("Yes, but I doubt they'll be of much use. That machine is beyond the subject entirely."),
		T_MARISA("Ugh, yer probably right, as usual."),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/01", {
		T_NARRATOR("Marisa closed the book and dropped it to the floor, with a loud noise."),
		T_PATCHOULI("Mind your manners! That’s a very rare tome!"),
		T_MARISA("I resolved the incident, but now we got that big tower just sittin’ there!"),
		T_MARISA("And the whole mountain's been quarantined, too! It just ain’t satisfyin’, y’know?"),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/01", {
		T_FLANDRE("I could destroy it! Make it go BOOM!"),
		T_MARISA("Gah! … now what did I say about sneakin’ up on people, Flan?"),
		T_FLANDRE("Sorry!"),
		T_PATCHOULI("Thank you for the offer, Flandre, but no."),
		T_FLANDRE("Awww… how boring…"),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/02", {
		T_NARRATOR("After having tea, Marisa wandered the stacks, looking for inspiration."),
		T_NARRATOR("Somehow, she tripped on her own feet, and landed face-first against a particularly hard and pointy book."),
		T_NARRATOR("Or… was it a book? She wasn’t quite sure."),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/02", {
		T_NARRATOR("After some time, she realized it was a ‘Smart Device’ of some kind, like a handheld computer."),
		T_NARRATOR("This one was far more advanced than any she'd seen from the Outside World, including that young occultist’s phone."),
		T_NARRATOR("It behaved both as a solid and a liquid, able to rapidly change shapes by applying pressure to certain sides, into different form factors: a phone, a book, a digital typewriter with a keyboard…"),
		T_NARRATOR("Once she figured out how to make its screen display something, a title of a textbook appeared suddenly:"),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/02", {
		T_NARRATOR("Practical & Advanced Computational Applications of the Grand Unified Theory"),
		T_NARRATOR("— by Usami Renko"),
		{ 0 },
	}},
	{ "cutscenes/marisa_bad/02", {
		T_CENTERED("— Bad End —", "Try to reach the end without using a Continue!"),
		{ 0 },
	}},
	{ NULL }
}
