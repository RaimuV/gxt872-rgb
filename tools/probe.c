/*
 * probe.c — discovery tool for the Trust GXT 872 Xyra vendor HID interface.
 *
 * The keyboard exposes, on interface 1 (usage page 0xFF00, usage 0x01):
 *   - feature report 5:  5 bytes  (probably mode/brightness/speed command)
 *   - feature report 6:  519 bytes (173 LEDs × 3 channels, probably the
 *                                   full per-LED RGB buffer)
 *   - input report 6:    7 bytes  (probably status/replies)
 *
 * Usage:
 *   probe <hidraw>                  info + read & analyse reports 5 and 6
 *   probe <hidraw> --read ID LEN    read feature report ID (data length LEN)
 *   probe <hidraw> --color RRGGBB   write report 6 filled with one colour
 *   probe <hidraw> --write ID HEX   write feature report ID (HEX = data)
 *
 * Build: gcc -O2 -Wall -Wextra probe.c -o probe
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/hidraw.h>

#define REPORT6_DATA_LEN 519
#define REPORT5_DATA_LEN 5

static void hexdump(const unsigned char *p, size_t n, size_t cols)
{
	for (size_t i = 0; i < n; i++) {
		printf("%02x ", p[i]);
		if ((i + 1) % cols == 0)
			printf("\n");
	}
	if (n % cols)
		printf("\n");
}

/* Interpret the 519-byte buffer as 173 RGB (or BGR) triples and print a
 * summary plus the first triples, to spot the colour layout. */
static void analyze_rgb(const unsigned char *p, size_t n)
{
	size_t triples = n / 3;
	unsigned char distinct[256] = { 0 };
	size_t n_distinct = 0;
	size_t zeros = 0, ffs = 0;
	size_t i;

	for (i = 0; i < n; i++) {
		if (!distinct[p[i]])
			n_distinct++;
		distinct[p[i]] = 1;
		if (p[i] == 0x00)
			zeros++;
		if (p[i] == 0xff)
			ffs++;
	}

	printf("bytes: %zu, LED triples: %zu, distinct values: %zu, "
	       "zeros: %zu, 0xff: %zu\n",
	       n, triples, n_distinct, zeros, ffs);

	printf("first 12 triples as RGB:");
	for (i = 0; i < 12 && (i + 1) * 3 <= n; i++)
		printf("  #%02zu:%02x%02x%02x", i, p[i*3], p[i*3+1], p[i*3+2]);
	printf("\n");
	printf("first 12 triples as BGR:");
	for (i = 0; i < 12 && (i + 1) * 3 <= n; i++)
		printf("  #%02zu:%02x%02x%02x", i, p[i*3+2], p[i*3+1], p[i*3]);
	printf("\n");
}

static int hid_open(const char *path, int *fd)
{
	*fd = open(path, O_RDWR);
	if (*fd < 0) {
		fprintf(stderr, "probe: open %s: %s\n", path, strerror(errno));
		return -1;
	}
	return 0;
}

static int read_feature(int fd, unsigned char id, unsigned char *buf,
			size_t data_len)
{
	size_t total = data_len + 1;
	int rc;

	memset(buf, 0, total);
	buf[0] = id;
	rc = ioctl(fd, HIDIOCGFEATURE(total), buf);
	if (rc < 0) {
		fprintf(stderr, "probe: HIDIOCGFEATURE(id=%u, len=%zu): %s\n",
			id, data_len, strerror(errno));
		return -1;
	}
	return 0;
}

static int write_feature(int fd, unsigned char id, const unsigned char *data,
			 size_t data_len)
{
	size_t total = data_len + 1;
	unsigned char *buf = malloc(total);
	int rc;

	if (!buf) {
		fprintf(stderr, "probe: out of memory\n");
		return -1;
	}
	buf[0] = id;
	memcpy(buf + 1, data, data_len);
	rc = ioctl(fd, HIDIOCSFEATURE(total), buf);
	if (rc < 0)
		fprintf(stderr, "probe: HIDIOCSFEATURE(id=%u, len=%zu): %s\n",
			id, data_len, strerror(errno));
	free(buf);
	return rc < 0 ? -1 : 0;
}

static int hex_nibble(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static size_t parse_hex(const char *s, unsigned char *out, size_t max)
{
	size_t len = 0;

	while (*s) {
		if (*s == ' ' || *s == ':') {
			s++;
			continue;
		}
		if (len >= max)
			return (size_t)-1;
		int hi = hex_nibble(*s++);
		if (!*s)
			return (size_t)-1;
		int lo = hex_nibble(*s++);
		if (hi < 0 || lo < 0)
			return (size_t)-1;
		out[len++] = (unsigned char)((hi << 4) | lo);
	}
	return len;
}

static void show_info(int fd)
{
	struct hidraw_devinfo info;
	char name[128] = { 0 };

	if (ioctl(fd, HIDIOCGRAWINFO, &info) == 0)
		printf("bus %04x vid %04x pid %04x\n",
		       info.bustype, info.vendor, info.product);
	if (ioctl(fd, HIDIOCGRAWNAME(sizeof(name)), name) >= 0)
		printf("name: %s\n", name);
}

static void usage(const char *argv0)
{
	fprintf(stderr,
		"Usage:\n"
		"  %s <hidraw>                    info + read reports 5 and 6\n"
		"  %s <hidraw> --read ID LEN      read feature report\n"
		"  %s <hidraw> --color RRGGBB     write report 6 (uniform)\n"
		"  %s <hidraw> --write ID HEX     write feature report\n",
		argv0, argv0, argv0, argv0);
}

int main(int argc, char **argv)
{
	int fd;
	int rc = 1;

	if (argc < 2) {
		usage(argv[0]);
		return 2;
	}

	if (hid_open(argv[1], &fd))
		return 1;
	show_info(fd);

	if (argc == 2) {
		unsigned char buf[REPORT6_DATA_LEN + 1];

		printf("--- feature report 5 ---\n");
		if (read_feature(fd, 5, buf, REPORT5_DATA_LEN) == 0)
			hexdump(buf + 1, REPORT5_DATA_LEN, 16);

		printf("--- feature report 6 (%d bytes) ---\n",
		       REPORT6_DATA_LEN);
		if (read_feature(fd, 6, buf, REPORT6_DATA_LEN) == 0) {
			hexdump(buf + 1, 48, 16);
			printf("...\n");
			analyze_rgb(buf + 1, REPORT6_DATA_LEN);
		}
		rc = 0;
	} else if (argc == 4 && !strcmp(argv[2], "--read")) {
		unsigned long id = strtoul(argv[3], NULL, 0);
		size_t len = id == 5 ? REPORT5_DATA_LEN : REPORT6_DATA_LEN;
		unsigned char buf[len + 1];

		if (read_feature(fd, (unsigned char)id, buf, len) == 0) {
			hexdump(buf + 1, len, 16);
			rc = 0;
		}
	} else if (argc == 4 && !strcmp(argv[2], "--color")) {
		unsigned char color[3];
		unsigned char data[REPORT6_DATA_LEN];
		size_t i;

		if (parse_hex(argv[3], color, 3) != 3) {
			fprintf(stderr, "probe: bad colour %s\n", argv[3]);
			goto out;
		}
		for (i = 0; i < REPORT6_DATA_LEN; i++)
			data[i] = color[i % 3];
		if (write_feature(fd, 6, data, REPORT6_DATA_LEN) == 0)
			rc = 0;
	} else if (argc == 5 && !strcmp(argv[2], "--write")) {
		unsigned long id = strtoul(argv[3], NULL, 0);
		unsigned char data[4096];
		size_t len = parse_hex(argv[4], data, sizeof(data));

		if (len == (size_t)-1) {
			fprintf(stderr, "probe: bad hex data\n");
			goto out;
		}
		if (write_feature(fd, (unsigned char)id, data, len) == 0)
			rc = 0;
	} else {
		usage(argv[0]);
	}

out:
	close(fd);
	return rc;
}
