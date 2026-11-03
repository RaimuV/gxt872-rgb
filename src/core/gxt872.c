/*
 * gxt872.c — core implementation: hidraw discovery and the LED feature
 * report. No dependencies beyond libc + linux/hidraw.h.
 */
#include "gxt872.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/hidraw.h>

#define GXT872_VID 0x145f
#define GXT872_PID 0x0367

#define REPORT_LEN 520
#define REPORT_ID 0x06
#define CMD_SET_COLOR 0x0a
#define COLOR_OFFSET 29

struct gxt872_device {
	int fd;
	char path[64];
};

/* ------------------------------------------------------------------ */
/* Discovery                                                          */
/* ------------------------------------------------------------------ */

/* Read a sysfs attribute of a hidraw node into buf. */
static int read_attr(const char *hidraw, const char *attr, char *buf,
		     size_t len)
{
	char path[256];
	FILE *f;
	size_t n;

	snprintf(path, sizeof(path), "/sys/class/hidraw/%s/device/%s", hidraw,
		 attr);
	f = fopen(path, "re");
	if (!f)
		return -1;
	n = fread(buf, 1, len - 1, f);
	fclose(f);
	if (n == 0)
		return -1;
	buf[n] = '\0';
	/* trim trailing newline */
	while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
		buf[--n] = '\0';
	return 0;
}

/* A matching node has HID_ID 0003:0000145F:00000367 and HID_PHYS ending
 * in "/input1" (the vendor interface). */
static int is_target(const char *hidraw)
{
	char id[512], phys[512];
	unsigned int bus = 0, vid = 0, pid = 0;
	size_t plen;

	if (read_attr(hidraw, "uevent", id, sizeof(id)))
		return 0;
	/* uevent contains several lines; look for HID_ID */
	char *line = strstr(id, "HID_ID=");
	if (!line)
		return 0;
	if (sscanf(line, "HID_ID=%x:%x:%x", &bus, &vid, &pid) != 3)
		return 0;
	if (vid != GXT872_VID || pid != GXT872_PID)
		return 0;

	if (read_attr(hidraw, "uevent", phys, sizeof(phys)))
		return 0;
	line = strstr(phys, "HID_PHYS=");
	if (!line)
		return 0;
	/* trim the line to its end */
	char *nl = strchr(line, '\n');
	if (nl)
		*nl = '\0';
	plen = strlen(line);
	return plen >= 7 && strcmp(line + plen - 7, "/input1") == 0;
}

int gxt872_enumerate(char paths[][64], int max)
{
	DIR *d = opendir("/sys/class/hidraw");
	struct dirent *de;
	int n = 0;

	if (!d)
		return -1;
	while ((de = readdir(d)) != NULL) {
		if (strncmp(de->d_name, "hidraw", 6) != 0)
			continue;
		if (n >= max)
			break;
		if (!is_target(de->d_name))
			continue;
		snprintf(paths[n], 64, "/dev/%s", de->d_name);
		n++;
	}
	closedir(d);
	return n;
}

/* ------------------------------------------------------------------ */
/* Device                                                             */
/* ------------------------------------------------------------------ */

struct gxt872_device *gxt872_open(const char *path)
{
	struct gxt872_device *dev;

	dev = calloc(1, sizeof(*dev));
	if (!dev)
		return NULL;

	if (path) {
		snprintf(dev->path, sizeof(dev->path), "%s", path);
	} else {
		/* enumerate needs a small array; keep it simple */
		char list[4][64];
		int n = gxt872_enumerate(list, 4);

		if (n <= 0) {
			free(dev);
			return NULL;
		}
		snprintf(dev->path, sizeof(dev->path), "%s", list[0]);
	}

	dev->fd = open(dev->path, O_RDWR | O_CLOEXEC);
	if (dev->fd < 0) {
		free(dev);
		return NULL;
	}
	return dev;
}

void gxt872_close(struct gxt872_device *dev)
{
	if (!dev)
		return;
	if (dev->fd >= 0)
		close(dev->fd);
	free(dev);
}

const char *gxt872_path(const struct gxt872_device *dev)
{
	return dev ? dev->path : "";
}

/* HIDIOCSFEATURE(len) */
#define HIDIOCSFEATURE_REQ(len) \
	(((3u) << 30) | ((len) << 16) | ((unsigned)'H' << 8) | 0x06)

static int send_report(struct gxt872_device *dev, const unsigned char *buf,
		       size_t len)
{
	return ioctl(dev->fd, HIDIOCSFEATURE_REQ(len), buf);
}

int gxt872_set_color(struct gxt872_device *dev, uint8_t r, uint8_t g,
		     uint8_t b)
{
	unsigned char report[REPORT_LEN];

	if (!dev)
		return -1;

	memset(report, 0, sizeof(report));
	report[0] = REPORT_ID;
	report[1] = CMD_SET_COLOR;
	report[4] = 0x01;
	report[7] = 0x02;
	report[COLOR_OFFSET + 0] = r;
	report[COLOR_OFFSET + 1] = g;
	report[COLOR_OFFSET + 2] = b;

	if (send_report(dev, report, sizeof(report)) < 0)
		return -1;
	return 0;
}

int gxt872_off(struct gxt872_device *dev)
{
	return gxt872_set_color(dev, 0, 0, 0);
}
