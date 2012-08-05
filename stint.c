#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <Imlib2.h>

void
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

	// Wait for button 1 press
	XEvent ev;
	do {
		XNextEvent(dpy, &ev);
	} while (ev.type != ButtonPress || ev.xbutton.button != 1);

	// Print colors until button 1 release
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

	// Release the pointer
	XUngrabPointer(dpy, CurrentTime);
out_free:
	// Clean up
	XFreeCursor(dpy, cross);

	XCloseDisplay(dpy);
	return rv;
}
