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

struct named_effect {
	const char *name;
	enum gxt872_effect value;
};

static const struct named_effect named_effects[] = {
	{ "static", GXT872_EFFECT_STATIC },
	{ "breathing", GXT872_EFFECT_BREATHING },
	{ "rainbow-cycle", GXT872_EFFECT_RAINBOW_CYCLE },
	{ "horizontal-ripple", GXT872_EFFECT_HORIZONTAL_RIPPLE },
	{ "raindrops", GXT872_EFFECT_RAINDROPS },
	{ "ripple-burst", GXT872_EFFECT_RIPPLE_BURST },
	{ "sparkle", GXT872_EFFECT_SPARKLE },
	{ "snake", GXT872_EFFECT_SNAKE },
	{ "rainbow-wave", GXT872_EFFECT_RAINBOW_WAVE },
	{ "reactive", GXT872_EFFECT_REACTIVE },
	{ "sine-wave", GXT872_EFFECT_SINE_WAVE },
	{ "rainbow-wheel", GXT872_EFFECT_RAINBOW_WHEEL },
	{ "rainbow-waterfall", GXT872_EFFECT_RAINBOW_WATERFALL },
	{ "bloom", GXT872_EFFECT_BLOOM },
	{ "custom", GXT872_EFFECT_CUSTOM },
};

static void usage(const char *argv0)
{
	fprintf(stderr,
		"Usage:\n"
		"  %s --color RRGGBB|#RRGGBB|NAME   set a solid colour\n"
		"  %s --effect NAME|0xNN            select a lighting effect\n"
		"  %s --brightness 0..3             brightness\n"
		"  %s --speed 0..4                  animation speed\n"
		"  %s --off                         turn the backlight off\n"
		"  %s --list                        list matching devices\n"
		"  %s --device PATH ...             use a specific hidraw node\n"
		"\n"
		"Colours:  red green blue white yellow magenta cyan orange purple pink\n"
		"Effects:  static breathing rainbow-cycle horizontal-ripple raindrops\n"
		"          ripple-burst sparkle snake rainbow-wave reactive sine-wave\n"
		"          rainbow-wheel rainbow-waterfall bloom off\n"
		"\n"
		"Options can be combined, e.g.:\n"
		"  %s --color 00FF80 --effect breathing --brightness 2 --speed 3\n",
		argv0, argv0, argv0, argv0, argv0, argv0, argv0, argv0);
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

static int parse_effect(const char *s, enum gxt872_effect *out)
{
	size_t i;

	if (!strncasecmp(s, "0x", 2)) {
		char *end = NULL;
		unsigned long v = strtoul(s, &end, 0);

		if (end && *end == '\0') {
			*out = (enum gxt872_effect)v;
			return 0;
		}
		return -1;
	}
	for (i = 0; i < sizeof(named_effects) / sizeof(named_effects[0]);
	     i++) {
		if (!strcasecmp(s, named_effects[i].name)) {
			*out = named_effects[i].value;
			return 0;
		}
	}
	return -1;
}

int main(int argc, char **argv)
{
	const char *device = NULL;
	const char *color = NULL;
	const char *effect = NULL;
	int brightness = -1, speed = -1;
	int do_off = 0, do_list = 0;
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--color") && i + 1 < argc)
			color = argv[++i];
		else if (!strcmp(argv[i], "--effect") && i + 1 < argc)
			effect = argv[++i];
		else if (!strcmp(argv[i], "--brightness") && i + 1 < argc)
			brightness = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--speed") && i + 1 < argc)
			speed = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--off"))
			do_off = 1;
		else if (!strcmp(argv[i], "--list"))
			do_list = 1;
		else if (!strcmp(argv[i], "--device") && i + 1 < argc)
			device = argv[++i];
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			usage(argv[0]);
			return 0;
		} else if (!strcmp(argv[i], "--version")) {
			printf("gxt872-rgb 0.2.0\n");
			return 0;
		} else if (argv[i][0] == '#') {
			color = argv[i];
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

	if (!color && !effect && !do_off && brightness < 0 && speed < 0) {
		usage(argv[0]);
		return 2;
	}

	unsigned char rgb[3] = { 0xff, 0xff, 0xff };
	enum gxt872_effect eff = GXT872_EFFECT_STATIC;

	if (color && parse_color(color, rgb)) {
		fprintf(stderr, "invalid colour: %s\n", color);
		return 2;
	}
	if (effect && parse_effect(effect, &eff)) {
		fprintf(stderr, "invalid effect: %s\n", effect);
		return 2;
	}
	if (brightness > GXT872_BRIGHTNESS_MAX || speed > GXT872_SPEED_MAX) {
		fprintf(stderr, "brightness 0..%d, speed 0..%d\n",
			GXT872_BRIGHTNESS_MAX, GXT872_SPEED_MAX);
		return 2;
	}

	struct gxt872_device *dev = gxt872_open(device);

	if (!dev) {
		fprintf(stderr, "cannot open the keyboard%s%s\n",
			device ? ": " : " (is it connected? permissions?)",
			device ? device : "");
		return 1;
	}

	int rc = 0;

	if (do_off)
		rc = gxt872_off(dev);
	else {
		if (color) {
			rc = gxt872_set_color(dev, rgb[0], rgb[1], rgb[2]);
			if (rc)
				goto out;
		}
		if (effect) {
			rc = gxt872_set_effect(dev, eff);
			if (rc)
				goto out;
		}
		if (brightness >= 0) {
			rc = gxt872_set_brightness(dev, brightness);
			if (rc)
				goto out;
		}
		if (speed >= 0)
			rc = gxt872_set_speed(dev, speed);
	}

	if (rc == 0)
		printf("ok on %s\n", gxt872_path(dev));
	else
		fprintf(stderr, "failed to send the report on %s\n",
			gxt872_path(dev));

out:
	gxt872_close(dev);
	return rc == 0 ? 0 : 1;
}
