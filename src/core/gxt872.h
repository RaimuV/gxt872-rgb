/*
 * gxt872.h — core API for the Trust GXT 872 Xyra RGB keyboard.
 *
 * The keyboard exposes a vendor HID interface (usage page 0xFF00) with a
 * 520-byte feature report (report ID 6). The protocol was reverse
 * engineered by capturing the vendor software:
 *
 *   report[0]      = 0x06              report ID
 *   report[1]      = 0x0a              command "set LED colour"
 *   report[4]      = 0x01
 *   report[7]      = 0x02
 *   report[29..31] = R, G, B
 *   rest           = 0x00
 *
 * Sending this one feature report changes the whole keyboard to a solid
 * colour. Works both on the wired interface and through the 2.4 GHz dongle.
 */
#ifndef GXT872_H
#define GXT872_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gxt872_device;

/* Open the keyboard. If path is NULL the first matching device is used
 * (-1 interface 1, vendor 0x145f / product 0x0367). Returns NULL on error. */
struct gxt872_device *gxt872_open(const char *path);

void gxt872_close(struct gxt872_device *dev);

/* Return the path of the opened device (e.g. "/dev/hidraw8"). */
const char *gxt872_path(const struct gxt872_device *dev);

/* Set a solid colour on all keys. */
int gxt872_set_color(struct gxt872_device *dev, uint8_t r, uint8_t g,
		     uint8_t b);

/* Convenience: turn the backlight off (solid black). */
int gxt872_off(struct gxt872_device *dev);

/* Enumerate the matching hidraw paths. Returns the number written, or -1. */
int gxt872_enumerate(char paths[][64], int max);

#ifdef __cplusplus
}
#endif

#endif /* GXT872_H */
