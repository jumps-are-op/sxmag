
#define DEFAULTPIXEL 0x181818
#define MAGSTEP 0.03f
#define KEY(s, k, func, a) { .state = s, .key = XK_##k, .f = func, .arg = a }
#define NOMOD 0

struct KeyBind keybinds[] = {
	KEY(ControlMask, Up,   scale, { .f = MAGSTEP }),
	KEY(ControlMask, Down, scale, { .f = -MAGSTEP }),

	KEY(ShiftMask|ControlMask, Up,   scale, { .f = MAGSTEP*2 }),
	KEY(ShiftMask|ControlMask, Down, scale, { .f = -MAGSTEP*2 }),

	KEY(ShiftMask, Left,  movex, { .i = -40 }),
	KEY(ShiftMask, Down,  movey, { .i = 40 }),
	KEY(ShiftMask, Up,    movey, { .i = -40 }),
	KEY(ShiftMask, Right, movex, { .i = 40 }),

	KEY(NOMOD, Left,  movex, { .i = -10 }),
	KEY(NOMOD, Down,  movey, { .i = 10 }),
	KEY(NOMOD, Up,    movey, { .i = -10 }),
	KEY(NOMOD, Right, movex, { .i = 10 }),
	KEY(NOMOD, Escape, stop, { 0 }),
};

/* Scale at startup (>1 = big, <1 = small) */
static double zoom = 1;

/* Size of a single pixel */
typedef uint32_t Pixel;
