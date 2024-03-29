/* BY: Jumps Are Op. (jumpsareop@gmail.com)
 * LICENSE: GPLv3-or-later
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#define LEN(x) (sizeof(x)/sizeof(x[0]))

static Display *dpy;
static int scr;
static GC gc;
static Window root, win;
static Atom wmdeletewin;
static double cx = 0, cy = 0, lastx, lasty, curx = 0, cury = 0;
static long w, h, ch, cw;
static bool started = false, holding = false, running;

union Arg {
	float f;
	int i;
	unsigned u;
};

struct KeyBind {
	void(*f)(union Arg);
	union Arg arg;
	unsigned int state;
	KeySym key;
};

static void scale(union Arg arg);
static void movex(union Arg arg);
static void movey(union Arg arg);
static void stop(union Arg arg);

#include "config.h"

static void XInit(void);
static void magnify(const Pixel*, Pixel*, long, long, double);
static void cmessage(XEvent*);
static void motion(XEvent*);
static void button(XEvent*);
static void unbutton(XEvent*);
static void keys(XEvent*);

static void(*eventhandler[LASTEvent])(XEvent*) = {
	[ClientMessage] = cmessage,
	[MotionNotify] = motion,
	[ButtonPress] = button,
	[ButtonRelease] = unbutton,
	[KeyPress] = keys
};

int main(void){
	long x, y, lastcx = cx, lastcy = cy, lasts = zoom;
	Pixel *in, *pout;
	XImage *out;
	XEvent ev;
	XWindowAttributes attr;

	XInit();

	XGetWindowAttributes(dpy, root, &attr);
	w = attr.width;
	h = attr.height;
	cx = lastx = curx = cw = w/2;
	cy = lasty = cury = ch = h/2;

	out = XGetImage(dpy, root, 0, 0, w, h, AllPlanes, ZPixmap);
	pout = (Pixel*)(out->data);
	in = malloc(w * h * sizeof(Pixel));
	if(!in){
		perror("malloc");
		return 1;
	}

	for(y = 0; y < h; y++)
		for(x = 0; x < w; x++)
			in[y*w + x] = pout[y*w + x];

	magnify(in, pout, cx, cy, zoom);

	XGetWindowAttributes(dpy, win, &attr);
	XPutImage(dpy, win, gc, out, 0, 0, 0, 0, attr.width, attr.height);

	/* Moved to here so the window wont flash at the start */
	XSync(dpy, False);
	XMapWindow(dpy, win);

	running = true;
	while(running){
		if(cx != lastcx || cy != lastcy || lasts != zoom)
			magnify(in, pout, cx, cy, zoom);
		lastcx = cx;
		lastcy = cy;
		lasts = zoom;
		XGetWindowAttributes(dpy, win, &attr);
		XPutImage(
			dpy, win, gc, out, 0, 0, 0, 0,
			attr.width, attr.height
		);

		do{
			XNextEvent(dpy, &ev);
			if(eventhandler[ev.type])
				eventhandler[ev.type](&ev);
		}while(XPending(dpy));
	}

	free(in);
	XDestroyImage(out);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
	return 0;
}

static void scale(union Arg arg){
	zoom += arg.f;
	if(zoom < 0)
		zoom = 0.1f;
}

static void movex(union Arg arg){
	cx += arg.i;
}

static void movey(union Arg arg){
	cy += arg.i;
}

static void stop(union Arg arg){
	running = false;
}

static void XInit(void){
	Atom wmstate, wmfullscreen;

	dpy = XOpenDisplay(NULL);
	if(!dpy){
		fputs("Failed to initialize display\n", stderr);
		exit(1);
	}

	scr = DefaultScreen(dpy);

	gc = DefaultGC(dpy, scr);

	root = DefaultRootWindow(dpy);
	if(root == None){
		fputs("No root window found\n", stderr);
		XCloseDisplay(dpy);
		exit(1);
	}

	win = XCreateSimpleWindow(
		dpy, root, 0, 0, 800, 600, 0, 0, DEFAULTPIXEL
	);
	if(win == None){
		fputs("Failed to create window\n", stderr);
		XCloseDisplay(dpy);
		exit(1);
	}

	XSelectInput(dpy, win, ExposureMask | KeyPressMask |
	                       ButtonPressMask | ButtonReleaseMask |
	                       FocusChangeMask | PointerMotionMask);
        XStoreName(dpy, win, "SXMag");

	wmstate = XInternAtom(dpy, "_NET_WM_STATE", True);
	wmfullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", True);
	wmdeletewin = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(dpy, win, &wmdeletewin, 1);
	XChangeProperty(dpy, win, wmstate, XA_ATOM, 32,
	                PropModeReplace, (unsigned char *)&wmfullscreen, 1);
}

static void magnify(const Pixel *in, Pixel *out, long cx, long cy, double s){
	long x, y, ix, iy;
	Pixel p;

	for(y = 0; y < h; y++){
		for(x = 0; x < w; x++){
			ix = (x-cw)/s + cx;
			iy = (y-ch)/s + cy;
			if(ix < 0 || ix >= w || iy < 0 || iy >= h)
				p = DEFAULTPIXEL;
			else
				p = in[iy*w + ix];

			out[y*w + x] = p;
		}
	}
}

static void cmessage(XEvent *e){
	if(e->xclient.data.l[0] == wmdeletewin)
		running = false;
}

static void motion(XEvent *e){
	curx = e->xmotion.x;
	cury = e->xmotion.y;

	if(!holding)
		return;

	if(started){
		started = false;
		lastx = curx;
		lasty = cury;
		return;
	}

	cx += (lastx - curx)/zoom;
	cy += (lasty - cury)/zoom;
	lastx = curx;
	lasty = cury;
}

static void button(XEvent *e){
	double c;
	switch(e->xbutton.button){
		case Button1:
			started = true;
			holding = true;
			return;
		case Button3:
			running = false;
			return;
		case Button4:
			c = MAGSTEP/(zoom*(zoom+MAGSTEP));
			cx += (curx-cw)*c;
			cy += (cury-ch)*c;
			zoom += MAGSTEP;
			return;
		case Button5:
			if(zoom <= MAGSTEP+0.1f){
				zoom = 0.1f;
			}else{
				c = MAGSTEP/(zoom*(zoom-MAGSTEP));
				cx -= (curx-cw)*c;
				cy -= (cury-ch)*c;
				zoom -= MAGSTEP;
			}
			return;
	}
}

static void unbutton(XEvent *e){
	if(e->xbutton.button == Button1)
		holding = false;
}

static void keys(XEvent *e){
	size_t i;
	unsigned int state = e->xkey.state;

	KeySym keysym = XLookupKeysym(&e->xkey, 0);
	if(!keysym)
		return;

	for(i = 0; i < LEN(keybinds); i++){
		if(keybinds[i].key == keysym && keybinds[i].state == state){
			keybinds[i].f(keybinds[i].arg);
			break;
		}
	}
}
