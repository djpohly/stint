/*
 * stint - simple, suckless-style color grabber for X11
 *
 * Copyright (C) 2012 Devin J. Pohly
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#define HELP_PARAM "--help"
#define HELP_SHORT_PARAM "-h"
#define X_PARAM "-x"
#define Y_PARAM "-y"

void
help()
{
	printf("Usage: stint [OPTIONS]\n");
	printf("Simple, suckless-style color grabber for X11\n");
	printf("\n");

	printf("When run, waits for the user to press the left mouse button.  As long as the\n\
button is held down, it will continue to print the color under the pointer in\n\
decimal \"RRR GGG BBB\" format.  Exits when the button is released.\n");
	printf("\n");

	printf("\t%s, %s\tShow this help\n", HELP_SHORT_PARAM, HELP_PARAM);
	printf("\t%s <x>\tUse x location instead of cursor x position (requires %s)\n", X_PARAM, Y_PARAM);
	printf("\t%s <y>\tUse y location instead of cursor y position (requires %s)\n", Y_PARAM, X_PARAM);
}

void
print_pixel(Display *dpy, Window root, int x, int y)
{
	XColor c;

	// Grab a 1x1 screenshot located at (x, y) and find the color
	c.pixel = XGetPixel(XGetImage(dpy, root, x, y, 1, 1, AllPlanes,
				ZPixmap), 0, 0);
	XQueryColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &c);

	// What color is it?
	printf("%d %d %d\n", c.red >> 8, c.green >> 8, c.blue >> 8);
	fflush(stdout);
}

int
get_loc(char *loc_param, int *loc, char *value_param, char *value)
{
	char *endptr;

	if (strcmp(loc_param, value_param) == 0) {
		*loc = strtol(value, &endptr, 0);
		if (errno != 0) {
			perror("strtol");
			return 1;
		}
		if (endptr == value) {
			fprintf(stderr, "no digits were found for %s\n", loc_param);
			return 1;
		}
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	int rv = 0;
	int fixed_pos = false;
	int x = -1;
	int y = -1;
	int ret;

	//Display help
	if (argc > 1 && ((strcmp(argv[1], HELP_PARAM) == 0) ||
			(strcmp(argv[1], HELP_SHORT_PARAM) == 0))) {
		help();
		return 0;
	}

	//Get arguments
	for (int i=0; i < argc; i++) {
		if (i != argc - 1) {
			ret = get_loc(X_PARAM, &x, argv[i], argv[i+1]);
			if (ret != 0)
				return ret;
			ret = get_loc(Y_PARAM, &y, argv[i], argv[i+1]);
			if (ret != 0)
				return ret;
		}
	}

	// Check arguments are valid
	if ((x == -1) ^ (y == -1)) {
		fprintf(stderr, "must supply either both x and y coordinates or none\n");
		return 3;
	}
	if (x != -1 && y != -1) {
		fixed_pos = true;
	}

	// Get display, root window, and crosshair cursor
	Display *dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "could not open display\n");
		return 1;
	}

	Window root = DefaultRootWindow(dpy);

	if(fixed_pos) {
		print_pixel(dpy, root, x, y);
		goto out_close;
	}

	Cursor cross = XCreateFontCursor(dpy, XC_crosshair);

	// Grab pointer clicking and motion events
	if (XGrabPointer(dpy, root, False, ButtonPressMask | Button1MotionMask | ButtonReleaseMask,
				GrabModeAsync, GrabModeAsync, root, cross, CurrentTime)) {
		fprintf(stderr, "could not grab pointer\n");
		rv = 1;
		goto out_free;
	}

	// Wait for button press
	XEvent ev;
	do {
		XNextEvent(dpy, &ev);
	} while (ev.type != ButtonPress);

	// Cancel if a non-Button1 button was pressed
	if (ev.xbutton.button != 1) {
		// Wait for release...
		int b = ev.xbutton.button;
		do {
			XNextEvent(dpy, &ev);
		} while (ev.type != ButtonRelease || ev.xbutton.button != b);

		// ... and cancel
		rv = 2;
		goto out_ungrab;
	}

	// Don't die immediately if we get a SIGPIPE
	struct sigaction sa_ign = { .sa_handler = SIG_IGN };
	sigaction(SIGPIPE, &sa_ign, NULL);

	// Print colors until Button1 is released
	while (ev.type != ButtonRelease || ev.xbutton.button != 1) {
		print_pixel(dpy, root, ev.xbutton.x_root, ev.xbutton.y_root);
		XNextEvent(dpy, &ev);
	}

out_ungrab:
	// Release the pointer grab
	XUngrabPointer(dpy, CurrentTime);
out_free:
	// Clean up
	XFreeCursor(dpy, cross);
out_close:
	XCloseDisplay(dpy);
	return rv;
}
