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
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <Imlib2.h>

static void
print_pixel(Window root, int x, int y)
{
	Imlib_Color color;
	Imlib_Image img;

	// Grab a 1x1 screenshot located at (x, y)
	imlib_context_set_drawable(root);
	img = imlib_create_image_from_drawable(None, x, y, 1, 1, 0);

	// What color is it?
	imlib_context_set_image(img);
	imlib_image_query_pixel(0, 0, &color);
	printf("#%02x%02x%02x\n", color.red, color.green, color.blue);
}

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

	// Set up Imlib2
	imlib_context_set_display(dpy);
	imlib_context_set_visual(DefaultVisual(dpy, DefaultScreen(dpy)));

	// Grab pointer clicking and motion events
	if (XGrabPointer(dpy, root, False, ButtonPressMask | Button1MotionMask |
			ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
			root, cross, CurrentTime)) {
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
		goto out_ungrab;
	}

	// Print colors until Button1 is released
	int done = 0;
	while (!done) {
		XNextEvent(dpy, &ev);

		switch (ev.type) {
			case ButtonRelease:
				if (ev.xbutton.button != 1)
					break;
				done = 1;
			case MotionNotify:
				print_pixel(root, ev.xbutton.x_root,
						ev.xbutton.y_root);
				break;
		}
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
