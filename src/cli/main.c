/* gxt872-rgb — command line frontend for the Trust GXT 872 Xyra RGB. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gxt872.h"

struct named_color {
	const char *name;
	unsigned char r, g, b;
};

static const struct named_color named_colors[] = {
	{ "red", 0xff, 0x00, 0x00 },   { "green", 0x00, 0xff, 0x00 },
	{ "blue", 0x00, 0x00, 0xff },  { "white", 0xff, 0xff, 0xff },
	{ "yellow", 0xff, 0xff, 0x00 },{ "magenta", 0xff, 0x00, 0xff },
	{ "cyan", 0x00, 0xff, 0xff },  { "orange", 0xff, 0x80, 0x00 },
	{ "purple", 0x80, 0x00, 0xff },{ "pink", 0xff, 0x40, 0x80 },
};

static void usage(const char *argv0)
{
	fprintf(stderr,
		"Usage:\n"
		"  %s --color RRGGBB|#RRGGBB|NAME   set a solid colour\n"
		"  %s --off                         turn the backlight off\n"
		"  %s --list                        list matching devices\n"
		"  %s --device PATH --color ...     use a specific hidraw node\n"
		"\n"
		"Colour names: red green blue white yellow magenta cyan orange\n"
		"              purple pink\n",
		argv0, argv0, argv0, argv0);
}

static int parse_color(const char *s, unsigned char *rgb)
{
	size_t i;

	if (*s == '#')
		s++;
	if (strlen(s) == 6) {
		char *end = NULL;
		unsigned long v = strtoul(s, &end, 16);

		if (end && *end == '\0') {
			rgb[0] = (unsigned char)(v >> 16);
			rgb[1] = (unsigned char)(v >> 8);
			rgb[2] = (unsigned char)v;
			return 0;
		}
		return -1;
	}
	for (i = 0; i < sizeof(named_colors) / sizeof(named_colors[0]); i++) {
		if (!strcasecmp(s, named_colors[i].name)) {
			rgb[0] = named_colors[i].r;
			rgb[1] = named_colors[i].g;
			rgb[2] = named_colors[i].b;
			return 0;
		}
	}
	return -1;
}

int main(int argc, char **argv)
{
	const char *device = NULL;
	const char *color = NULL;
	int do_off = 0, do_list = 0;
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--color") && i + 1 < argc)
			color = argv[++i];
		else if (!strcmp(argv[i], "--off"))
			do_off = 1;
		else if (!strcmp(argv[i], "--list"))
			do_list = 1;
		else if (!strcmp(argv[i], "--device") && i + 1 < argc)
			device = argv[++i];
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage(argv[0]);
			return 0;
		} else if (argv[i][0] == '#') {
			color = argv[i];
		} else if (!strcmp(argv[i], "--version")) {
			printf("gxt872-rgb 0.1.0\n");
			return 0;
		} else {
			usage(argv[0]);
			return 2;
		}
	}

	if (do_list) {
		char paths[8][64];
		int n = gxt872_enumerate(paths, 8);

		if (n <= 0) {
			fprintf(stderr, "no GXT 872 Xyra found\n");
			return 1;
		}
		for (i = 0; i < n; i++)
			printf("%s\n", paths[i]);
		return 0;
	}

	if (!color && !do_off) {
		usage(argv[0]);
		return 2;
	}

	unsigned char rgb[3] = { 0, 0, 0 };

	if (color && parse_color(color, rgb)) {
		fprintf(stderr, "invalid colour: %s\n", color);
		return 2;
	}

	struct gxt872_device *dev = gxt872_open(device);

	if (!dev) {
		fprintf(stderr, "cannot open the keyboard%s%s\n",
			device ? ": " : " (is it connected? permissions?)",
			device ? device : "");
		return 1;
	}

	int rc = gxt872_set_color(dev, rgb[0], rgb[1], rgb[2]);

	if (rc == 0)
		printf("colour set to %02x%02x%02x on %s\n", rgb[0], rgb[1],
		       rgb[2], gxt872_path(dev));
	else
		fprintf(stderr, "failed to send the report on %s\n",
			gxt872_path(dev));

	gxt872_close(dev);
	return rc == 0 ? 0 : 1;
}
