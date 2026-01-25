#include "i18n/format_strings.h"
#include "log.h"
#include "rwops/rwops_stdiofp.h"
#include "util.h"

static bool test_is_format_specifier(void) {
	const char *format_specifiers[] = {
		// valid
		"%09d  au",
		"%-#10x",
		"%'.2f",
		"%2$*3$.3f",
		"%b",
		"%m%",
		"%+10.4e",
		"%#06o",
		"%#-12.0g",
		// invalid
		"",
		"%",
		"%%d",
		" %d",
		"%0",
		"100%",
	};
	int expected_lengths[] = {
	4, 6, 5, 9, 2, 2, 7, 5, 8, 0, 0, 0, 0, 0, 0,
	};

        assert(ARRAY_SIZE(expected_lengths) == ARRAY_SIZE(format_specifiers));
	for(int i = 0; i < ARRAY_SIZE(expected_lengths); i++) {
		if(is_format_specifier(format_specifiers[i]) != expected_lengths[i]) {
			log_info("is_format_specifier(\"%s\") != %d", format_specifiers[i], expected_lengths[i]);
			return false;
		}
	}
	return true;
}

static bool test_match_format_strings(void) {
	const char *format_strings[][2] = {
		// mismatch
		{"", "abc %X"},
		{"%m", "%X"},
		{" %2", " %2d"},
		{"%d%%%i", "%i%%%d"},
		{"%ä", "%d咕%"},
		{"%%s", "%s"},
		{"%2$*3$.3f%", "%2$*3$.2f"},
		{"%s %s %d", "%s %d %s"},
		// match
		{"", ""},
		{"", "abc"},
		{" %d ", "%%aouuu %d %%"},
		{" %2$*3$.3f%s%%", "%2$*3$.3f %s"},
		{"%%", ""},
		{"Can't replay this stage: unknown stage ID %X", "Kann Stage nicht wiedergeben: unbekannte Stage ID %X"},
		{"咕咕%g", "咕%g咕" },
	};

	for(int i = 0; i < ARRAY_SIZE(format_strings); i++) {
		bool expected = i > 7;
		if(match_format_strings(format_strings[i][0], format_strings[i][1]) != expected) {
			log_info("match_format_strings(\"%s\", \"%s\") != %d", format_strings[i][0], format_strings[i][1], expected);
			return false;
		}
		if(match_format_strings(format_strings[i][1], format_strings[i][0]) != expected) {
			log_info("match_format_strings(\"%s\", \"%s\") != %d", format_strings[i][1], format_strings[i][0], expected);
			return false;
		}
	}
	return true;
}

int main(int argc, char **argv) {
	log_init(LOG_ALL);
	atexit(log_shutdown);
	log_add_output(LOG_ALL, SDL_RWFromFP(stderr, false), log_formatter_console);

	if(!test_is_format_specifier()) {
		return 1;
	}
	if(!test_match_format_strings()) {
		return 1;
	}
	return 0;
}
