/*
 * gxt872.h — core API for the Trust GXT 872 Xyra RGB keyboard.
 *
 * The keyboard exposes a vendor HID interface (usage page 0xFF00) with a
 * 520-byte feature report (report ID 6). Two commands are used:
 *
 *   report[1] = 0x0a  colour:      R,G,B at bytes 29..31
 *   report[1] = 0x04  config:      effect at byte 18, brightness at byte 66,
 *                                  speed at byte 69 (0x07 + 0x10 * level)
 *
 * Reverse engineered by capturing the vendor software (Wine + usbmon) and
 * diffing the packets. Sending only the needed report is enough.
 */
#ifndef GXT872_H
#define GXT872_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Lighting effects, values as stored in the config report. */
enum gxt872_effect {
	GXT872_EFFECT_STATIC = 0x01,
	GXT872_EFFECT_BREATHING = 0x02,
	GXT872_EFFECT_RAINBOW_CYCLE = 0x03,
	GXT872_EFFECT_HORIZONTAL_RIPPLE = 0x04,
	GXT872_EFFECT_RAINDROPS = 0x05,
	GXT872_EFFECT_RIPPLE_BURST = 0x07,
	GXT872_EFFECT_SPARKLE = 0x08,
	GXT872_EFFECT_SNAKE = 0x0a,
	GXT872_EFFECT_RAINBOW_WAVE = 0x0b,
	GXT872_EFFECT_REACTIVE = 0x0c,
	GXT872_EFFECT_SINE_WAVE = 0x0d,
	GXT872_EFFECT_RAINBOW_WHEEL = 0x0f,
	GXT872_EFFECT_RAINBOW_WATERFALL = 0x10,
	GXT872_EFFECT_BLOOM = 0x11,
	/* 0x15 enables the per-key "custom" mode; the vendor app sends a
	 * companion 0x06 report with it. /off is implemented as black. */
	GXT872_EFFECT_CUSTOM = 0x15,
};

#define GXT872_BRIGHTNESS_MIN 0
#define GXT872_BRIGHTNESS_MAX 3
#define GXT872_SPEED_MIN 0
#define GXT872_SPEED_MAX 4

struct gxt872_device;

struct gxt872_state {
	uint8_t r, g, b;
	uint8_t effect;
	uint8_t brightness;
	uint8_t speed;
};

/* Open the keyboard. If path is NULL the first matching device is used.
 * Returns NULL on error. */
struct gxt872_device *gxt872_open(const char *path);

void gxt872_close(struct gxt872_device *dev);

/* Path of the opened device (e.g. "/dev/hidraw8"). */
const char *gxt872_path(const struct gxt872_device *dev);

/* Copy the last known settings (colour/effect/brightness/speed). */
void gxt872_get_state(const struct gxt872_device *dev,
		      struct gxt872_state *state);

/* Set a solid colour on all keys. */
int gxt872_set_color(struct gxt872_device *dev, uint8_t r, uint8_t g,
		     uint8_t b);

/* Select a lighting effect. */
int gxt872_set_effect(struct gxt872_device *dev, enum gxt872_effect effect);

/* Brightness 0..3. */
int gxt872_set_brightness(struct gxt872_device *dev, int level);

/* Speed 0..4 (only meaningful for animated effects). */
int gxt872_set_speed(struct gxt872_device *dev, int level);

/* Turn the backlight off. */
int gxt872_off(struct gxt872_device *dev);

/* Enumerate the matching hidraw paths. Returns the number written, or -1. */
int gxt872_enumerate(char paths[][64], int max);

#ifdef __cplusplus
}
#endif

#endif /* GXT872_H */
