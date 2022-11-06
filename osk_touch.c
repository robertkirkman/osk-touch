
/*
 * osk_touch.c : framebuffer on-screen keyboard, touchscreen version for Steam Deck tty
 *
 * Copyright (C) 2014 Rafael Ignacio Zurita <rafaelignacio.zurita@gmail.com>
 *
 * Based on : http://thiemonge.org/getting-started-with-uinput
 * and www.cubieforums.com/index.php?topic=33.0
 * and fb_drawimage() from https://svn.mcs.anl.gov/repos/ZeptoOS/trunk/BGP/packages/busybox/src/miscutils/fbsplash.c
 * and evtest from https://cgit.freedesktop.org/evtest/tree/evtest.c
 * and fbkeyboard from https://github.com/julianwi/fbkeyboard/blob/master/fbkeyboard.c
 *
 *   osk_touch.c is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *    You should have received a copy of the GNU Library General Public
 *    License along with this software (check COPYING); if not, write to the Free
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <linux/ioctl.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#define FRAMEBUFFER_FILE "/dev/fb0"
#define KEYBOARD_FILE "/dev/uinput"
#define TOUCHSCREEN_FILE "/dev/input/event5"

#define TOTAL_KEYS 59

typedef struct
{
	unsigned short key;
	unsigned int y1, x1, y2, x2;
} Key;

/* TODO: add escape */
Key keyboard[TOTAL_KEYS] = {
	{KEY_GRAVE, 5, 285, 46, 350},
	{KEY_1, 51, 285, 135, 350},
	{KEY_2, 141, 285, 225, 350},
	{KEY_3, 230, 285, 314, 350},
	{KEY_4, 320, 285, 404, 350},
	{KEY_5, 409, 285, 493, 350},
	{KEY_6, 499, 285, 583, 350},
	{KEY_7, 588, 285, 673, 350},
	{KEY_8, 678, 285, 762, 350},
	{KEY_9, 768, 285, 852, 350},
	{KEY_0, 857, 285, 941, 350},
	{KEY_MINUS, 947, 285, 1031, 350},
	{KEY_EQUAL, 1036, 285, 1120, 350},
	{KEY_BACKSPACE, 1126, 285, 1274, 350},
	{KEY_TAB, 5, 215, 93, 280},
	{KEY_Q, 98, 215, 184, 280},
	{KEY_W, 189, 215, 274, 280},
	{KEY_E, 280, 215, 365, 280},
	{KEY_R, 371, 215, 456, 280},
	{KEY_T, 462, 215, 547, 280},
	{KEY_Y, 553, 215, 638, 280},
	{KEY_U, 643, 215, 729, 280},
	{KEY_I, 734, 215, 820, 280},
	{KEY_O, 825, 215, 910, 280},
	{KEY_P, 916, 215, 1001, 280},
	{KEY_LEFTBRACE, 1007, 215, 1092, 280},
	{KEY_RIGHTBRACE, 1098, 215, 1183, 280},
	{KEY_BACKSLASH, 1189, 215, 1274, 280},
	{KEY_CAPSLOCK, 5, 144, 123, 209},
	{KEY_A, 129, 144, 214, 209},
	{KEY_S, 219, 144, 305, 209},
	{KEY_D, 310, 144, 395, 209},
	{KEY_F, 401, 144, 486, 209},
	{KEY_G, 491, 144, 577, 209},
	{KEY_H, 582, 144, 667, 209},
	{KEY_J, 673, 144, 758, 209},
	{KEY_K, 763, 144, 848, 209},
	{KEY_L, 854, 144, 939, 209},
	{KEY_SEMICOLON, 945, 144, 1030, 209},
	{KEY_APOSTROPHE, 1035, 144, 1120, 209},
	{KEY_ENTER, 1126, 144, 1274, 209},
	{KEY_LEFTSHIFT, 5, 74, 182, 139},
	{KEY_Z, 188, 74, 273, 139},
	{KEY_X, 278, 74, 364, 139},
	{KEY_C, 369, 74, 455, 139},
	{KEY_V, 460, 74, 546, 139},
	{KEY_B, 551, 74, 637, 139},
	{KEY_N, 642, 74, 727, 139},
	{KEY_M, 733, 74, 818, 139},
	{KEY_COMMA, 824, 74, 909, 139},
	{KEY_DOT, 915, 74, 1000, 139},
	{KEY_SLASH, 1006, 74, 1091, 139},
	{KEY_RIGHTSHIFT, 1097, 74, 1274, 139},
	{KEY_LEFTCTRL, 5, 3, 108, 68},
	{KEY_SPACE, 114, 3, 838, 68},
	{KEY_LEFT, 843, 3, 947, 68},
	{KEY_RIGHT, 952, 3, 1056, 68},
	{KEY_UP, 1061, 3, 1165, 68},
	{KEY_DOWN, 1170, 3, 1274, 68}};

/*
 *	Draw image from PPM file
 */
/* TODO: fix segfault at 1233rd row */
static void fb_drawimage(const char *filename, char *fbp, int xo, int yo, int bpp, int length, int xres, unsigned *img_height, unsigned *img_width)
{

	char head[256];
	char s[80];
	FILE *theme_file;
	unsigned char *pixline;
	unsigned i, j, line_size;

	memset(head, 0, sizeof(head));
	theme_file = fopen(filename, "r");

	/* parse ppm header */
	while (1)
	{
		if (fgets(s, sizeof(s), theme_file) == NULL)
		{
			printf("bad PPM file '%s'", filename);
			exit(1);
		}

		if (s[0] == '#')
			continue;

		if (strlen(head) + strlen(s) >= sizeof(head))
		{
			printf("bad PPM file '%s'", filename);
			exit(1);
		}

		strcat(head, s);
		if (head[0] != 'P' || head[1] != '6')
		{
			printf("bad PPM file '%s'", filename);
			exit(1);
		}

		/* width, height, max_color_val */
		if (sscanf(head, "P6 %u %u %u", img_width, img_height, &i) == 3)
			break;
		/* TODO: i must be <= 255! */
	}

	line_size = *img_width * 3;

	long int location = 0;
	pixline = malloc(line_size);
	for (j = 0; j < *img_height; j++)
	{
		unsigned char *pixel = pixline;

		if (fread(pixline, 1, line_size, theme_file) != line_size)
		{
			printf("bad PPM file '%s'", filename);
			exit(1);
		}

		for (i = 0; i < *img_width; i++)
		{
			pixel += 3;
			location = (i + xo) * (bpp / 8) + (j + yo) * length;

			*(fbp + location) = (((unsigned)pixel[0]));
			*(fbp + location + 1) = (((unsigned)pixel[1]));
			*(fbp + location + 2) = (((unsigned)pixel[2]));
			*(fbp + location + 3) = -1; /* No transparency */
		}
	}
	free(pixline);
	fclose(theme_file);
}

void send_key(int fd, unsigned short code)
{
	struct input_event ev;
	ev.type = EV_KEY;
	ev.code = code;
	ev.value = 1;
	if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
		fprintf(stderr, "error: sending uinput event\n");
	ev.value = 0;
	if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
		fprintf(stderr, "error: sending uinput event\n");
	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
		fprintf(stderr, "error: sending uinput event\n");
}

unsigned short get_key(unsigned int abs_x, unsigned int abs_y)
{
	for (int i = 0; i < TOTAL_KEYS; i++)
		if (abs_x >= keyboard[i].x1 && abs_x <= keyboard[i].x2 && abs_y >= keyboard[i].y1 && abs_y <= keyboard[i].y2)
			return keyboard[i].key;
	return 0;
}

int main(void)
{

	int fbfd = 0;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long int screensize = 0;
	char *fbp = 0;
	/* img dimensions set in fb_drawimage */
	unsigned img_height = 0;
	unsigned img_width = 0;

	/* uinput init */
	int i, fd;
	struct uinput_user_dev device;
	memset(&device, 0, sizeof device);

	fd = open(KEYBOARD_FILE, O_WRONLY);
	if (fd == -1)
	{
		perror("Error: cannot open uinput device\n");
		exit(1);
	}
	strcpy(device.name, "osk-touch");

	device.id.bustype = BUS_USB;
	device.id.vendor = 1;
	device.id.product = 1;
	device.id.version = 1;

	if (write(fd, &device, sizeof(device)) != sizeof(device))
	{
		fprintf(stderr, "error setting up uinput device\n");
	}

	if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
		fprintf(stderr, "error: SET_EVBIT EV_KEY\n");

	if (ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0)
		fprintf(stderr, "error: SET_EVBIT EV_SYN\n");

	for (i = 0; i < TOTAL_KEYS; i++)
		if (ioctl(fd, UI_SET_KEYBIT, keyboard[i].key) < 0)
			fprintf(stderr, "error evbit key\n");

	if (ioctl(fd, UI_SET_EVBIT, EV_REL) < 0)
		fprintf(stderr, "error evbit rel\n");

	for (i = REL_X; i < REL_MAX; i++)
		if (ioctl(fd, UI_SET_RELBIT, i) < 0)
			fprintf(stderr, "error setrelbit %d\n", i);

	if (ioctl(fd, UI_DEV_CREATE) < 0)
	{
		fprintf(stderr, "error creating uinput device\n");
	}
	/* end uinput init */

	/* init framebuffer */
	/* Open the file for reading and writing */
	fbfd = open(FRAMEBUFFER_FILE, O_RDWR);
	if (fbfd == -1)
	{
		perror("Error: cannot open framebuffer device");
		exit(2);
	}
	printf("The framebuffer device was opened successfully.\n");

	/* Get fixed screen information */
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
	{
		perror("Error reading fixed information");
		exit(3);
	}

	/* Get variable screen information */
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
	{
		perror("Error reading variable information");
		exit(4);
	}

	printf("%dx%d, %dbpp, line length %d\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, finfo.line_length);

	/* Figure out the size of the screen in bytes */
	screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

	/* Map the device to memory */
	fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
					   fbfd, 0);
	if (fbp == MAP_FAILED)
	{
		perror("Error: failed to map framebuffer device to memory");
		exit(5);
	}
	printf("The framebuffer device was mapped to memory successfully.\n");
	/* end init framebuffer */

	/* init touchscreen */
	int fdm;
	struct input_event ev;
	if ((fdm = open(TOUCHSCREEN_FILE, O_RDONLY | O_NONBLOCK)) == -1)
	{
		printf("Device open ERROR\n");
		exit(6);
	}
	fd_set rdfs;
	FD_ZERO(&rdfs);
	FD_SET(fdm, &rdfs);
	/* end init touchscreen */

	int pressed = 0, released = 0, abs_x = 0, abs_y = 0, shift = 0, caps = 0, ctrl = 0;
	unsigned short pressedkey = 0;

	/* TODO: exit condition */
	while (1)
	{
		/* draw keyboard texture */
		/* TODO: swap texture for capslock/shift/ctrl */
		fb_drawimage("deck_keyboard.ppm", fbp, vinfo.xoffset, vinfo.yoffset, vinfo.bits_per_pixel, finfo.line_length, vinfo.xres, &img_height, &img_width);

		/* read and parse input events from touchscreen for coordinates and touch state */
		/* TODO: multitouch, n-key rollover (?) */
		select(fdm + 1, &rdfs, NULL, NULL, NULL);
		released = 0;
		while (read(fdm, &ev, sizeof(struct input_event)) && !(ev.type == EV_SYN && ev.code == SYN_REPORT))
		{
			if (ev.type == EV_ABS)
			{
				switch (ev.code)
				{
				case ABS_MT_POSITION_X:
					abs_x = ev.value;
					pressed = 1;
					break;
				case ABS_MT_POSITION_Y:
					abs_y = ev.value;
					pressed = 1;
					break;
				case ABS_MT_TRACKING_ID:
					if (ev.value == -1)
					{
						pressed = 0;
						released = 1;
					}
					break;
				}
			}
			else if (ev.type == EV_SYN && ev.code == SYN_MT_REPORT)
			{
				pressed = 0;
				released = 1;
			}
		}

		/* push corresponding keys */
		if (released && pressedkey)
		{
			if (pressedkey == KEY_LEFTSHIFT || pressedkey == KEY_RIGHTSHIFT)
			{
				shift ^= 1;
				ev.type = EV_KEY;
				ev.code = KEY_LEFTSHIFT;
				ev.value = shift;
				if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
					fprintf(stderr, "error sending uinput event\n");
			}
			else if (pressedkey == KEY_LEFTCTRL)
			{
				ctrl ^= 1;
				ev.type = EV_KEY;
				ev.code = KEY_LEFTCTRL;
				ev.value = ctrl;
				if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
					fprintf(stderr, "error sending uinput event\n");
			}
			else
			{
				if (pressedkey == KEY_CAPSLOCK)
					caps ^= 1;
				send_key(fd, pressedkey);
			}
		}

		/* cross-reference state with table */
		pressedkey = 0;
		if (pressed)
			pressedkey = get_key(abs_x, abs_y);
	}

	close(fd);

	ioctl(fdm, EVIOCGRAB, (void *)0);
	close(fdm);

	munmap(fbp, screensize);
	close(fbfd);

	return 0;
}
