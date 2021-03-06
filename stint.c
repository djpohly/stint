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
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

int
main(int argc, char *argv[])
{
	int rv = 0;

	// Get display, root window, and crosshair cursor
	Display *dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "could not open display\n");
		return 1;
	}

	Window root = DefaultRootWindow(dpy);
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
		XColor c;

		// Grab a 1x1 screenshot located at (x, y) and find the color
		c.pixel = XGetPixel(XGetImage(dpy, root, ev.xbutton.x_root, ev.xbutton.y_root,
					1, 1, AllPlanes, ZPixmap), 0, 0);
		XQueryColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &c);

		// What color is it?
		printf("%d %d %d\n", c.red >> 8, c.green >> 8, c.blue >> 8);
		fflush(stdout);

		XNextEvent(dpy, &ev);
	}

out_ungrab:
	// Release the pointer grab
	XUngrabPointer(dpy, CurrentTime);
out_free:
	// Clean up
	XFreeCursor(dpy, cross);

	XCloseDisplay(dpy);
	return rv;
}
