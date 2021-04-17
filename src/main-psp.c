//* File: main-psp.c */

/*
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */


/*
 * This file helps Angband work on the Sony Playstation Portable
 *
 * To use this file, use "Makefile.psp", which defines USE_PSP.
 */


#include "angband.h"

#ifdef USE_PSP

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <psppower.h>

#define ANGBAND_SCREEN_COLS 80
#define ANGBAND_SCREEN_ROWS 33

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

#define PSP_SCREEN_WIDTH 480
#define PSP_SCREEN_HEIGHT 272
#define PSP_LINE_SIZE 512
#define PSP_PIXEL_FORMAT 3

#include "main.h"
#define INTERNAL_MAIN

PSP_MODULE_INFO("Angband", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(0);
PSP_HEAP_SIZE_KB(20480); //!!

enum {EXEC_NORMAL, EXEC_SLEEP, EXEC_EXIT};
volatile char execution_state = EXEC_NORMAL;

typedef struct star_t_struct {
	struct star_t_struct* links[4];
	char macro[64];
	char desc[64];
} star_t;

// 3 display routines for the psp (slightly modified versions of the pspsdk ones)
void psp_screen_init();
void psp_put_char(int x, int y, unsigned long color, unsigned long bk_color, unsigned char ch);
void psp_put_string(int x, int y, unsigned long color, unsigned long bk_color, char* string);

// PSP implementations of standard functions
u64 psp_mktime(ScePspDateTime* time); // get number of seconds since 1970


// PSP versions of Angband routines
errr check_modification_date(int fd, cptr template_file); // check if template newer than .raw
void psp_plog(cptr str); //psp version of plog_aux, rough dump text to screen

// The standard Angband terminal hooks
static void Term_init_psp(term *t);
static void Term_nuke_psp(term *t);
static errr Term_user_psp(int n);
static errr Term_xtra_psp(int n, int v);
static errr Term_curs_psp(int x, int y);
static errr Term_wipe_psp(int x, int y, int n);
static errr Term_text_psp(int x, int y, int n, byte a, const char *cp);
static errr Term_pict_psp(int x, int y, int n, const byte *ap, const char *cp,
                          const byte *tap, const char *tcp);
static void term_data_link(int i);

// Refreshes n characters starting from row x, col y of the Angband display
void psp_refresh(int x, int y, int n);

// Draws the cursor
void psp_cursor_flip();

// Thread management routines
int psp_exit_callback(void); // exit the program on receiving the home button
void psp_callback_thread(void *arg); // runs the callback functions
void psp_timer_thread(void* arg); // blinks the cursor, updates power meter
void psp_input_thread(void* arg); // reports button presses to Angband
void psp_main_thread(void* arg); // thread in which Angband runs (necessary?)

// General input routines
int psp_analog_octant(int x, int y); // maps the analog stick to an 8 way pad
void psp_update_keys(); // gets the current button status

// Onscreen keyboard routines
void psp_show_osk(); // displays the onscreen keyboard
void psp_highlight_osk(int cx, int cy, int color); // highlight one part of the keyboard
void psp_hide_osk(); // erases the onscreen keyboard
char psp_do_osk(); // get a character from the user via the onscreen keyboard

// Macro routines
void psp_show_star(star_t* star); // displays the macro interface 
void psp_hide_star(); // erases the macro interface
void psp_save_star(int fff, star_t* star); // recursively saves a macro star to disk
void psp_save_macros(); // saves all macros to disk
void psp_load_star(int fff, star_t* star); // recursively loads a star from disk
void psp_load_macros(); // loads all macros from disk
void psp_modify_macro(); // allows the user to change macros and save the changes in game
void psp_do_macro(); // invokes a macro selected by the user

// Thread ids
int cb_thid, input_thid, timer_thid, main_thid;

// The palette to display text in 
static long pal[16] = {
				0x00000000, /* Black */
				0x00FFFFFF, /* White */
				0x00808080, /* Gray */
				0x000080FF, /* Orange */
				0x000000FF, /* Red */
				0x00408000, /* Green */
				0x00FF0000, /* Blue */
				0x00004080, /* Brown */
				0x00606060, /* Dark Gray */
				0x00A0A0A0, /* Light Gray */
				0x00FF00FF, /* Purple */
				0x0000FFFF, /* Yellow */
				0x004040FF, /* Light Red */
				0x0000FF00, /* Light Green */
				0x00FFFF00, /* Light Blue */
				0x004080C0  /* Light Brown */
};

unsigned long* vram_base;
bool screen_init = 0;

// Cursor state variables
bool curs_enable = 0, curs_on = 0;
unsigned char curs_back_attr, curs_x, curs_y;
char curs_back_char;

#define NUM_KEYS 12
int key_signals[NUM_KEYS] = {
				PSP_CTRL_SELECT, PSP_CTRL_START, 
				PSP_CTRL_UP, PSP_CTRL_RIGHT, PSP_CTRL_DOWN, PSP_CTRL_LEFT, 
				PSP_CTRL_LTRIGGER, PSP_CTRL_RTRIGGER,
				PSP_CTRL_TRIANGLE, PSP_CTRL_CIRCLE, PSP_CTRL_CROSS, PSP_CTRL_SQUARE};
enum {KEY_SELECT, KEY_START, KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_LEFT, 
			KEY_LTRIGGER, KEY_RTRIGGER, KEY_TRIANGLE, KEY_CIRCLE, KEY_CROSS, KEY_SQUARE};
enum {STICK_C, STICK_N, STICK_NE, STICK_E, STICK_SE, 
				STICK_S, STICK_SW, STICK_W, STICK_NW};

#define REPEAT_PERIOD 4
#define INITIAL_REPEAT_DELAY 20
#define HELD(x) (key_state[x] != -1)
#define PRESSED(x) (!key_state[x] || (key_state[x] > INITIAL_REPEAT_DELAY && !(key_state[x] % REPEAT_PERIOD)))

#define A_REPEAT_PERIOD 6
#define A_INITIAL_REPEAT_DELAY 12
#define A_HELD(x) (analog_octant == x)
#define A_PRESSED(x) (analog_octant == x && (!analog_state || (!(analog_state % A_REPEAT_PERIOD) && analog_state >= A_INITIAL_REPEAT_DELAY)))

/* Issue a '.' command before moving to run */
#define TRY_RUN if (HELD(KEY_RTRIGGER)) Term_keypress('.')


/*
 *  IO state.  
 *
 *  Remembers how long the keys and analog stick have been in a given state.
 *  key_state has one entry for each key_signal.  A given entry in key_state
 *  is -1 if the button is not pressed.  When pressed, this value counts up
 *  from 0 until the button is released.  The PRESSED macro uses this count to
 *  turn held keys into individual button presses at a given rate.
 */
int key_state[NUM_KEYS];
int analog_state = 0;
char analog_octant = STICK_C;

// The onscreen keyboard, regular and shifted (ends with spaces to align)
char keys[2][52] = {"`1234567890-=abcdefghijklmnopqrstuvwxyz[]\\;',./    ",
									"~!@#$%^&*()_+ABCDEFGHIJKLMNOPQRSTUVWXYZ{}|\":<>?     "};

// Position at which to draw the keyboard
#define KEYBOARD_COL 31
#define KEYBOARD_ROW 24

// Row number on which to write the macro instructions (horizontally centered)
#define INSTR_ROW 32

// The default macros
star_t star_ll = {{NULL, NULL, NULL, NULL}, "mbf", "Teleport"};
star_t star_lu = {{NULL, NULL, NULL, NULL}, "mab", "Det.Monster"};
star_t star_lr = {{NULL, NULL, NULL, NULL}, "mcf", "Identify"};
star_t star_ld = {{NULL, NULL, NULL, NULL}, "mah", "Det.Traps"};
star_t star_ul = {{NULL, NULL, NULL, NULL}, "R&\r", "Rest Status"};
star_t star_uu = {{NULL, NULL, NULL, NULL}, "R100\r", "Rest 100"};
star_t star_ur = {{NULL, NULL, NULL, NULL}, "maf", "Cure Light"};
star_t star_ud = {{NULL, NULL, NULL, NULL}, "mad", "Light Area"};
star_t star_rl = {{NULL, NULL, NULL, NULL}, "mcd", "Fire Bolt"};
star_t star_ru = {{NULL, NULL, NULL, NULL}, "mai", "Stink Cloud"};
star_t star_rr = {{NULL, NULL, NULL, NULL}, "mbb", "L. Bolt"};
star_t star_rd = {{NULL, NULL, NULL, NULL}, "mda", "Frost Ball"};
star_t star_dl = {{NULL, NULL, NULL, NULL}, "k-y", "Destroy"};
star_t star_du = {{NULL, NULL, NULL, NULL}, "0100s", "Search"};
star_t star_dr = {{NULL, NULL, NULL, NULL}, "mcc", "Stone->Mud"};
star_t star_dd = {{NULL, NULL, NULL, NULL}, "0100D", "Disarm"};
star_t star_l  = {{&star_ll, &star_lu, &star_lr, &star_ld}, "mac", "Phase Door"};
star_t star_u  = {{&star_ul, &star_uu, &star_ur, &star_ud}, "R*\r", "Rest HP/MP"};
star_t star_r  = {{&star_rl, &star_ru, &star_rr, &star_rd}, "maa", "M. Missile"};
star_t star_d  = {{&star_dl, &star_du, &star_dr, &star_dd}, "0100o", "Pick Lock"};
star_t star    = {{&star_l, &star_u, &star_r, &star_d}, "*tmaa5", "Auto M.M."};

/*
 * A font that is 99% the same as the one from the ps2dev.org pspsdk.  
 * Modifications are:
 *   50% hash block: 0xB1
 *   Circle Button: 0x10 and 0x11   (use both together)
 *   Cross Button: 0x12 and 0x13    
 *   Triangle Button: 0x14 and 0x15 
 *   Square Button: 0x16 and 0x17
 *   Up Arrow: 0x18
 *   Left Arrow: 0x19
 *   Down Arrow: 0x1A
 *   Right Arrow: 0x1B
 */
u8 font[]=
"\x00\x00\x00\x00\x00\x00\x00\x00\x3c\x42\xa5\x81\xa5\x99\x42\x3c"
"\x3c\x7e\xdb\xff\xff\xdb\x66\x3c\x6c\xfe\xfe\xfe\x7c\x38\x10\x00"
"\x10\x38\x7c\xfe\x7c\x38\x10\x00\x10\x38\x54\xfe\x54\x10\x38\x00"
"\x10\x38\x7c\xfe\xfe\x10\x38\x00\x00\x00\x00\x30\x30\x00\x00\x00"
"\xff\xff\xff\xe7\xe7\xff\xff\xff\x38\x44\x82\x82\x82\x44\x38\x00"
"\xc7\xbb\x7d\x7d\x7d\xbb\xc7\xff\x0f\x03\x05\x79\x88\x88\x88\x70"
"\x38\x44\x44\x44\x38\x10\x7c\x10\x30\x28\x24\x24\x28\x20\xe0\xc0"
"\x3c\x24\x3c\x24\x24\xe4\xdc\x18\x10\x54\x38\xee\x38\x54\x10\x00"
"\x0C\x10\x24\x28\x28\x24\x10\x0C\xC0\x20\x90\x50\x50\x90\x20\xC0"
"\x0C\x10\x28\x24\x24\x28\x10\x0C\xC0\x20\x50\x90\x90\x50\x20\xC0"
"\x0C\x10\x24\x24\x28\x2C\x10\x0C\xC0\x20\x90\x90\x50\xD0\x20\xC0"
"\x0C\x10\x2C\x28\x28\x2C\x10\x0C\xC0\x20\xD0\x50\x50\xD0\x20\xC0"
"\x20\x70\xA8\x20\x20\x20\x20\x00\x00\x20\x40\xfc\x40\x20\x00\x00"
"\x20\x20\x20\x20\xA8\x70\x20\x00\x00\x10\x08\xfc\x08\x10\x00\x00"
"\x81\x42\x24\x18\x18\x24\x42\x81\x01\x02\x04\x08\x10\x20\x40\x80"
"\x80\x40\x20\x10\x08\x04\x02\x01\x00\x10\x10\xff\x10\x10\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x20\x20\x20\x20\x00\x00\x20\x00"
"\x50\x50\x50\x00\x00\x00\x00\x00\x50\x50\xf8\x50\xf8\x50\x50\x00"
"\x20\x78\xa0\x70\x28\xf0\x20\x00\xc0\xc8\x10\x20\x40\x98\x18\x00"
"\x40\xa0\x40\xa8\x90\x98\x60\x00\x10\x20\x40\x00\x00\x00\x00\x00"
"\x10\x20\x40\x40\x40\x20\x10\x00\x40\x20\x10\x10\x10\x20\x40\x00"
"\x20\xa8\x70\x20\x70\xa8\x20\x00\x00\x20\x20\xf8\x20\x20\x00\x00"
"\x00\x00\x00\x00\x00\x20\x20\x40\x00\x00\x00\x78\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x60\x60\x00\x00\x00\x08\x10\x20\x40\x80\x00"
"\x70\x88\x98\xa8\xc8\x88\x70\x00\x20\x60\xa0\x20\x20\x20\xf8\x00"
"\x70\x88\x08\x10\x60\x80\xf8\x00\x70\x88\x08\x30\x08\x88\x70\x00"
"\x10\x30\x50\x90\xf8\x10\x10\x00\xf8\x80\xe0\x10\x08\x10\xe0\x00"
"\x30\x40\x80\xf0\x88\x88\x70\x00\xf8\x88\x10\x20\x20\x20\x20\x00"
"\x70\x88\x88\x70\x88\x88\x70\x00\x70\x88\x88\x78\x08\x10\x60\x00"
"\x00\x00\x20\x00\x00\x20\x00\x00\x00\x00\x20\x00\x00\x20\x20\x40"
"\x18\x30\x60\xc0\x60\x30\x18\x00\x00\x00\xf8\x00\xf8\x00\x00\x00"
"\xc0\x60\x30\x18\x30\x60\xc0\x00\x70\x88\x08\x10\x20\x00\x20\x00"
"\x70\x88\x08\x68\xa8\xa8\x70\x00\x20\x50\x88\x88\xf8\x88\x88\x00"
"\xf0\x48\x48\x70\x48\x48\xf0\x00\x30\x48\x80\x80\x80\x48\x30\x00"
"\xe0\x50\x48\x48\x48\x50\xe0\x00\xf8\x80\x80\xf0\x80\x80\xf8\x00"
"\xf8\x80\x80\xf0\x80\x80\x80\x00\x70\x88\x80\xb8\x88\x88\x70\x00"
"\x88\x88\x88\xf8\x88\x88\x88\x00\x70\x20\x20\x20\x20\x20\x70\x00"
"\x38\x10\x10\x10\x90\x90\x60\x00\x88\x90\xa0\xc0\xa0\x90\x88\x00"
"\x80\x80\x80\x80\x80\x80\xf8\x00\x88\xd8\xa8\xa8\x88\x88\x88\x00"
"\x88\xc8\xc8\xa8\x98\x98\x88\x00\x70\x88\x88\x88\x88\x88\x70\x00"
"\xf0\x88\x88\xf0\x80\x80\x80\x00\x70\x88\x88\x88\xa8\x90\x68\x00"
"\xf0\x88\x88\xf0\xa0\x90\x88\x00\x70\x88\x80\x70\x08\x88\x70\x00"
"\xf8\x20\x20\x20\x20\x20\x20\x00\x88\x88\x88\x88\x88\x88\x70\x00"
"\x88\x88\x88\x88\x50\x50\x20\x00\x88\x88\x88\xa8\xa8\xd8\x88\x00"
"\x88\x88\x50\x20\x50\x88\x88\x00\x88\x88\x88\x70\x20\x20\x20\x00"
"\xf8\x08\x10\x20\x40\x80\xf8\x00\x70\x40\x40\x40\x40\x40\x70\x00"
"\x00\x00\x80\x40\x20\x10\x08\x00\x70\x10\x10\x10\x10\x10\x70\x00"
"\x20\x50\x88\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xf8\x00"
"\x40\x20\x10\x00\x00\x00\x00\x00\x00\x00\x70\x08\x78\x88\x78\x00"
"\x80\x80\xb0\xc8\x88\xc8\xb0\x00\x00\x00\x70\x88\x80\x88\x70\x00"
"\x08\x08\x68\x98\x88\x98\x68\x00\x00\x00\x70\x88\xf8\x80\x70\x00"
"\x10\x28\x20\xf8\x20\x20\x20\x00\x00\x00\x68\x98\x98\x68\x08\x70"
"\x80\x80\xf0\x88\x88\x88\x88\x00\x20\x00\x60\x20\x20\x20\x70\x00"
"\x10\x00\x30\x10\x10\x10\x90\x60\x40\x40\x48\x50\x60\x50\x48\x00"
"\x60\x20\x20\x20\x20\x20\x70\x00\x00\x00\xd0\xa8\xa8\xa8\xa8\x00"
"\x00\x00\xb0\xc8\x88\x88\x88\x00\x00\x00\x70\x88\x88\x88\x70\x00"
"\x00\x00\xb0\xc8\xc8\xb0\x80\x80\x00\x00\x68\x98\x98\x68\x08\x08"
"\x00\x00\xb0\xc8\x80\x80\x80\x00\x00\x00\x78\x80\xf0\x08\xf0\x00"
"\x40\x40\xf0\x40\x40\x48\x30\x00\x00\x00\x90\x90\x90\x90\x68\x00"
"\x00\x00\x88\x88\x88\x50\x20\x00\x00\x00\x88\xa8\xa8\xa8\x50\x00"
"\x00\x00\x88\x50\x20\x50\x88\x00\x00\x00\x88\x88\x98\x68\x08\x70"
"\x00\x00\xf8\x10\x20\x40\xf8\x00\x18\x20\x20\x40\x20\x20\x18\x00"
"\x20\x20\x20\x00\x20\x20\x20\x00\xc0\x20\x20\x10\x20\x20\xc0\x00"
"\x40\xa8\x10\x00\x00\x00\x00\x00\x00\x00\x20\x50\xf8\x00\x00\x00"
"\x70\x88\x80\x80\x88\x70\x20\x60\x90\x00\x00\x90\x90\x90\x68\x00"
"\x10\x20\x70\x88\xf8\x80\x70\x00\x20\x50\x70\x08\x78\x88\x78\x00"
"\x48\x00\x70\x08\x78\x88\x78\x00\x20\x10\x70\x08\x78\x88\x78\x00"
"\x20\x00\x70\x08\x78\x88\x78\x00\x00\x70\x80\x80\x80\x70\x10\x60"
"\x20\x50\x70\x88\xf8\x80\x70\x00\x50\x00\x70\x88\xf8\x80\x70\x00"
"\x20\x10\x70\x88\xf8\x80\x70\x00\x50\x00\x00\x60\x20\x20\x70\x00"
"\x20\x50\x00\x60\x20\x20\x70\x00\x40\x20\x00\x60\x20\x20\x70\x00"
"\x50\x00\x20\x50\x88\xf8\x88\x00\x20\x00\x20\x50\x88\xf8\x88\x00"
"\x10\x20\xf8\x80\xf0\x80\xf8\x00\x00\x00\x6c\x12\x7e\x90\x6e\x00"
"\x3e\x50\x90\x9c\xf0\x90\x9e\x00\x60\x90\x00\x60\x90\x90\x60\x00"
"\x90\x00\x00\x60\x90\x90\x60\x00\x40\x20\x00\x60\x90\x90\x60\x00"
"\x40\xa0\x00\xa0\xa0\xa0\x50\x00\x40\x20\x00\xa0\xa0\xa0\x50\x00"
"\x90\x00\x90\x90\xb0\x50\x10\xe0\x50\x00\x70\x88\x88\x88\x70\x00"
"\x50\x00\x88\x88\x88\x88\x70\x00\x20\x20\x78\x80\x80\x78\x20\x20"
"\x18\x24\x20\xf8\x20\xe2\x5c\x00\x88\x50\x20\xf8\x20\xf8\x20\x00"
"\xc0\xa0\xa0\xc8\x9c\x88\x88\x8c\x18\x20\x20\xf8\x20\x20\x20\x40"
"\x10\x20\x70\x08\x78\x88\x78\x00\x10\x20\x00\x60\x20\x20\x70\x00"
"\x20\x40\x00\x60\x90\x90\x60\x00\x20\x40\x00\x90\x90\x90\x68\x00"
"\x50\xa0\x00\xa0\xd0\x90\x90\x00\x28\x50\x00\xc8\xa8\x98\x88\x00"
"\x00\x70\x08\x78\x88\x78\x00\xf8\x00\x60\x90\x90\x90\x60\x00\xf0"
"\x20\x00\x20\x40\x80\x88\x70\x00\x00\x00\x00\xf8\x80\x80\x00\x00"
"\x00\x00\x00\xf8\x08\x08\x00\x00\x84\x88\x90\xa8\x54\x84\x08\x1c"
"\x84\x88\x90\xa8\x58\xa8\x3c\x08\x20\x00\x00\x20\x20\x20\x20\x00"
"\x00\x00\x24\x48\x90\x48\x24\x00\x00\x00\x90\x48\x24\x48\x90\x00"
"\x28\x50\x20\x50\x88\xf8\x88\x00\x55\xAA\x55\xAA\x55\xAA\x55\xAA"
"\x28\x50\x00\x70\x20\x20\x70\x00\x28\x50\x00\x20\x20\x20\x70\x00"
"\x28\x50\x00\x70\x88\x88\x70\x00\x50\xa0\x00\x60\x90\x90\x60\x00"
"\x28\x50\x00\x88\x88\x88\x70\x00\x50\xa0\x00\xa0\xa0\xa0\x50\x00"
"\xfc\x48\x48\x48\xe8\x08\x50\x20\x00\x50\x00\x50\x50\x50\x10\x20"
"\xc0\x44\xc8\x54\xec\x54\x9e\x04\x10\xa8\x40\x00\x00\x00\x00\x00"
"\x00\x20\x50\x88\x50\x20\x00\x00\x88\x10\x20\x40\x80\x28\x00\x00"
"\x7c\xa8\xa8\x68\x28\x28\x28\x00\x38\x40\x30\x48\x48\x30\x08\x70"
"\x00\x00\x00\x00\x00\x00\xff\xff\xf0\xf0\xf0\xf0\x0f\x0f\x0f\x0f"
"\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x3c\x3c\x00\x00\x00\xff\xff\xff\xff\xff\xff\x00\x00"
"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\x0f\x0f\x0f\x0f\xf0\xf0\xf0\xf0"
"\xfc\xfc\xfc\xfc\xfc\xfc\xfc\xfc\x03\x03\x03\x03\x03\x03\x03\x03"
"\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x3f\x11\x22\x44\x88\x11\x22\x44\x88"
"\x88\x44\x22\x11\x88\x44\x22\x11\xfe\x7c\x38\x10\x00\x00\x00\x00"
"\x00\x00\x00\x00\x10\x38\x7c\xfe\x80\xc0\xe0\xf0\xe0\xc0\x80\x00"
"\x01\x03\x07\x0f\x07\x03\x01\x00\xff\x7e\x3c\x18\x18\x3c\x7e\xff"
"\x81\xc3\xe7\xff\xff\xe7\xc3\x81\xf0\xf0\xf0\xf0\x00\x00\x00\x00"
"\x00\x00\x00\x00\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x00\x00\x00\x00"
"\x00\x00\x00\x00\xf0\xf0\xf0\xf0\x33\x33\xcc\xcc\x33\x33\xcc\xcc"
"\x00\x20\x20\x50\x50\x88\xf8\x00\x20\x20\x70\x20\x70\x20\x20\x00"
"\x00\x00\x00\x50\x88\xa8\x50\x00\xff\xff\xff\xff\xff\xff\xff\xff"
"\x00\x00\x00\x00\xff\xff\xff\xff\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0"
"\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\xff\xff\xff\xff\x00\x00\x00\x00"
"\x00\x00\x68\x90\x90\x90\x68\x00\x30\x48\x48\x70\x48\x48\x70\xc0"
"\xf8\x88\x80\x80\x80\x80\x80\x00\xf8\x50\x50\x50\x50\x50\x98\x00"
"\xf8\x88\x40\x20\x40\x88\xf8\x00\x00\x00\x78\x90\x90\x90\x60\x00"
"\x00\x50\x50\x50\x50\x68\x80\x80\x00\x50\xa0\x20\x20\x20\x20\x00"
"\xf8\x20\x70\xa8\xa8\x70\x20\xf8\x20\x50\x88\xf8\x88\x50\x20\x00"
"\x70\x88\x88\x88\x50\x50\xd8\x00\x30\x40\x40\x20\x50\x50\x50\x20"
"\x00\x00\x00\x50\xa8\xa8\x50\x00\x08\x70\xa8\xa8\xa8\x70\x80\x00"
"\x38\x40\x80\xf8\x80\x40\x38\x00\x70\x88\x88\x88\x88\x88\x88\x00"
"\x00\xf8\x00\xf8\x00\xf8\x00\x00\x20\x20\xf8\x20\x20\x00\xf8\x00"
"\xc0\x30\x08\x30\xc0\x00\xf8\x00\x18\x60\x80\x60\x18\x00\xf8\x00"
"\x10\x28\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\xa0\x40"
"\x00\x20\x00\xf8\x00\x20\x00\x00\x00\x50\xa0\x00\x50\xa0\x00\x00"
"\x00\x18\x24\x24\x18\x00\x00\x00\x00\x30\x78\x78\x30\x00\x00\x00"
"\x00\x00\x00\x00\x30\x00\x00\x00\x3e\x20\x20\x20\xa0\x60\x20\x00"
"\xa0\x50\x50\x50\x00\x00\x00\x00\x40\xa0\x20\x40\xe0\x00\x00\x00"
"\x00\x38\x38\x38\x38\x38\x38\x00\x00\x00\x00\x00\x00\x00\x00";

// The character in the font that gives a solid block
#define BLOCK_CHAR 219

#ifdef CHECK_MODIFICATION_TIME
u64	psp_mktime(ScePspDateTime *t)
{
	int	m_to_d[12] =
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
	short	month, year;
	u64		result;

	month = t->month;
	year = t->year + month / 12 + 1900;
	month %= 12;
	if (month < 0)
	{
		year -= 1;
		month += 12;
	}
	result = (year - 1970) * 365 + (year - 1969) / 4 + m_to_d[month];
	result = (year - 1970) * 365 + m_to_d[month];
	if (month <= 1)
		year -= 1;
	result += (year - 1968) / 4;
	result -= (year - 1900) / 100;
	result += (year - 1600) / 400;
	result += t->day;
	result -= 1;
	result *= 24;
	result += t->hour;
	result *= 60;
	result += t->minute;
	result *= 60;
	result += t->second;

	return result;
}

errr check_modification_date(int fd, cptr template_file)
{
	char *p;
	char buf[1024];
	char bufRaw[1024];
	char fname[32];

	struct SceIoStat txt_stat, raw_stat;

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_EDIT, template_file);

	/* Build filename for RAW file */
	strnfmt(fname, sizeof(fname), "%s", template_file);

	/* Find last '.' */
	p = strrchr(fname, '.');

	/* Can't happen */
	if (p == NULL) return (-1);

	/* Substitute ".raw" for ".txt" */
	strcpy(p, ".raw");

	path_build(bufRaw, sizeof(bufRaw), ANGBAND_DIR_DATA, fname);

	/* Access stats on text file */
	if (sceIoGetstat(buf, &txt_stat))
	{
		/* No text file - continue */
	}

	/* Access stats on raw file */
	else if (sceIoGetstat(bufRaw, &raw_stat))
	{
		/* Error */
		return (-1);
	}
	/* Ensure text file is not newer than raw file */
	else if (psp_mktime(&txt_stat.st_mtime) > psp_mktime(&raw_stat.st_mtime))
	{
		/* Reprocess text file */
		return (-1);
	}

	return (0);
}

#endif

/* Another stdio function implementation I should be flogged for */
char * tmpnam(char * buffer){
	time_t t;
	int m;
	char *tmp = ralloc(512);
	SceIoStat stat;
	
	cptr scrap_path = ANGBAND_DIR;

	sceKernelLibcTime( &t);
	
	for (m = 0; m < 80; m++)
	{
		sprintf(tmp, "%s0x%08x.tmp", scrap_path, (int)t + m);

		if (sceIoGetstat(tmp, &stat) < 0)
		{
			break;
		}
	}

	if (m < 80)
	{
		return tmp;
		//strncpy(buf, tmp, max);
		return 0;
	}
	return 0;
}

void psp_plog(cptr str)
{
	pspDebugScreenSetXY(0,0);
	pspDebugScreenPrintf(str);
}
/*
 * Extra data to associate with each "window"
 *
 * Each "window" is represented by a "term_data" structure, which
 * contains a "term" structure, which contains a pointer (t->data)
 * back to the term_data structure.
 */

typedef struct term_data term_data;

struct term_data
{
	term t;

	/* Other fields if needed XXX XXX XXX */
};



/*
 * Number of "term_data" structures to support XXX XXX XXX
 *
 * Actually, MAX_TERM_DATA is now defined as eight in 'defines.h'.
 *
 * Since only one "term_data" structure is supported, a lot of
 * the things that would normally go into a "term_data" structure
 * could be made into global variables instead.
 */
#define MAX_PSP_TERM 1


// An array of "term_data" structures, one for each "sub-window"
static term_data data[MAX_PSP_TERM];


#if 0

/*
 * Often, it is helpful to create an array of "color data" containing
 * a representation of the "angband_color_table" array in some "local" form.
 *
 * Often, the "Term_xtra(TERM_XTRA_REACT, 0)" hook is used to initialize
 * "color_data" from "angband_color_table".  XXX XXX XXX
 */
static local_color_data_type color_data[256];

#endif



/*** Function hooks needed by "Term" ***/


/*
 * Init a new "term"
 *
 * This function should do whatever is necessary to prepare a new "term"
 * for use by the "z-term.c" package.  This may include clearing the window,
 * preparing the cursor, setting the font/colors, etc.  Usually, this
 * function does nothing, and the "init_xxx()" function does it all.
 */
static void Term_init_psp(term *t)
{
	/* The single screen is initialized by main(), not by Angband */
}



/*
 * Nuke an old "term"
 *
 * This function is called when an old "term" is no longer needed.  It should
 * do whatever is needed to clean up before the program exits, such as wiping
 * the screen, restoring the cursor, fixing the font, etc.  Often this function
 * does nothing and lets the operating system clean up when the program quits.
 */
static void Term_nuke_psp(term *t)
{
	/* There is only one static screen that is never destroyed */
}



/*
 * Do a "user action" on the current "term"
 *
 * This function allows the visual module to do implementation defined
 * things when the user activates the "system defined command" command.
 *
 * This function is normally not used.
 *
 * In general, this function should return zero if the action is successfully
 * handled, and non-zero if the action is unknown or incorrectly handled.
 */
static errr Term_user_psp(int n)
{
	/* Not doing anything here */
	return (1);
}


/*
 * Do a "special thing" to the current "term"
 *
 * This function must react to a large number of possible arguments, each
 * corresponding to a different "action request" by the "z-term.c" package,
 * or by the application itself.
 *
 * The "action type" is specified by the first argument, which must be a
 * constant of the form "TERM_XTRA_*" as given in "z-term.h", and the second
 * argument specifies the "information" for that argument, if any, and will
 * vary according to the first argument.
 *
 * In general, this function should return zero if the action is successfully
 * handled, and non-zero if the action is unknown or incorrectly handled.
 */
static errr Term_xtra_psp(int n, int v)
{
	/* Analyze */
	switch (n)
	{
		case TERM_XTRA_EVENT:
		{
			/*
			 * Process some pending events XXX XXX XXX
			 *
			 * Wait for at least one event if "v" is non-zero
			 * otherwise, if no events are ready, return at once.
			 * When "keypress" events are encountered, the "ascii"
			 * value corresponding to the key should be sent to the
			 * "Term_keypress()" function.  Certain "bizarre" keys,
			 * such as function keys or arrow keys, may send special
			 * sequences of characters, such as control-underscore,
			 * plus letters corresponding to modifier keys, plus an
			 * underscore, plus carriage return, which can be used by
			 * the main program for "macro" triggers.  This action
			 * should handle as many events as is efficiently possible
			 * but is only required to handle a single event, and then
			 * only if one is ready or "v" is true.
			 *
			 * This action is required.
			 */

			/*
			 * Actually, it's not.  Instead, IO is being fed to the queue 
			 * asynchronously.  But this is a great time to free up some 
			 * time slices for the other threads running.
			 *
			 * That said, I think this may be the place to insert escaped
			 * terminal commands via Term_keypress.  Doing it in psp_input_thread
			 * doesn't work at least...
			 */
			sceDisplayWaitVblankStart();

			return (0);
		}

		case TERM_XTRA_FLUSH:
		{
			/*
			 * Flush all pending events XXX XXX XXX
			 *
			 * This action should handle all events waiting on the
			 * queue, optionally discarding all "keypress" events,
			 * since they will be discarded anyway in "z-term.c".
			 *
			 * This action is required, but may not be "essential".
			 */

			/* Not in control of any events besides keypresses */
			return (0);
		}

		case TERM_XTRA_CLEAR:
		{
			/*
			 * Clear the entire window XXX XXX XXX
			 *
			 * This action should clear the entire window, and redraw
			 * any "borders" or other "graphic" aspects of the window.
			 *
			 * This action is required.
			 */

			int i, j;
			for (i = 0; i < ANGBAND_SCREEN_ROWS; i++) {
				for (j = 0; j < ANGBAND_SCREEN_COLS; j++) {
					psp_put_char(j * CHAR_WIDTH, i * CHAR_HEIGHT, 0xFFFFFF, 0x000000, ' ');
				}
			}

			return (0);
		}

		case TERM_XTRA_SHAPE:
		{
			/*
			 * Set the cursor visibility XXX XXX XXX
			 *
			 * This action should change the visibility of the cursor,
			 * if possible, to the requested value (0=off, 1=on)
			 *
			 * This action is optional, but can improve both the
			 * efficiency (and attractiveness) of the program.
			 */

			/* Cursor blink state controlled by psp_timer_thread */
			return (0);
		}

		case TERM_XTRA_FROSH:
		{
			/*
			 * Flush a row of output XXX XXX XXX
			 *
			 * This action should make sure that row "v" of the "output"
			 * to the window will actually appear on the window.
			 *
			 * This action is optional, assuming that "Term_text_xxx()"
			 * (and similar functions) draw directly to the screen, or
			 * that the "TERM_XTRA_FRESH" entry below takes care of any
			 * necessary flushing issues.
			 */

			/* Term_text_psp handles this */
			return (0);
		}

		case TERM_XTRA_FRESH:
		{
			/*
			 * Flush output XXX XXX XXX
			 *
			 * This action should make sure that all "output" to the
			 * window will actually appear on the window.
			 *
			 * This action is optional, assuming that "Term_text_xxx()"
			 * (and similar functions) draw directly to the screen, or
			 * that the "TERM_XTRA_FROSH" entry above takes care of any
			 * necessary flushing issues.
			 */

			/* Term_text_psp handles this */
			return (0);
		}

		case TERM_XTRA_NOISE:
		{
			/*
			 * Make a noise XXX XXX XXX
			 *
			 * This action should produce a "beep" noise.
			 *
			 * This action is optional, but convenient.
			 */

			/* Maybe someday */
			return (0);
		}

		/*case TERM_XTRA_SOUND:
		{
			/*
			 * Make a sound XXX XXX XXX
			 *
			 * This action should produce sound number "v", where the
			 * "name" of that sound is "sound_names[v]".  This method
			 * is still under construction.
			 *
			 * This action is optional, and not very important.
			 */

			/* 
			 * Do I really want to hear cheesy sound effects every time I 
			 * hit a monster? 
			 * */
		/*	return (0);
		}*/ //!!

		case TERM_XTRA_BORED:
		{
			/*
			 * Handle random events when bored XXX XXX XXX
			 *
			 * This action is optional, and normally not important
			 */

			/* TODO: Make the @ tap its toe */
			return (0);
		}

		case TERM_XTRA_REACT:
		{
			/*
			 * React to global changes XXX XXX XXX
			 *
			 * For example, this action can be used to react to
			 * changes in the global "angband_color_table[256][4]" array.
			 *
			 * This action is optional, but can be very useful for
			 * handling "color changes" and the "arg_sound" and/or
			 * "arg_graphics" options.
			 */

			/* Don't know what this is for really */
			return (0);
		}

		case TERM_XTRA_ALIVE:
		{
			/*
			 * Change the "hard" level XXX XXX XXX
			 *
			 * This action is used if the program changes "aliveness"
			 * by being either "suspended" (v=0) or "resumed" (v=1)
			 * This action is optional, unless the computer uses the
			 * same "physical screen" for multiple programs, in which
			 * case this action should clean up to let other programs
			 * use the screen, or resume from such a cleaned up state.
			 *
			 * This action is currently only used by "main-gcu.c",
			 * on UNIX machines, to allow proper "suspending".
			 */

			/* !!!! Suspending? This sounds important */
			return (0);
		}

		case TERM_XTRA_LEVEL:
		{
			/*
			 * Change the "soft" level XXX XXX XXX
			 *
			 * This action is used when the term window changes "activation"
			 * either by becoming "inactive" (v=0) or "active" (v=1)
			 *
			 * This action can be used to do things like activate the proper
			 * font / drawing mode for the newly active term window.  This
			 * action should NOT change which window has the "focus", which
			 * window is "raised", or anything like that.
			 *
			 * This action is optional if all the other things which depend
			 * on what term is active handle activation themself, or if only
			 * one "term_data" structure is supported by this file.
			 */

			/* Only one term_data for PSP */
			return (0);
		}

		case TERM_XTRA_DELAY:
		{
			/*
			 * Delay for some milliseconds XXX XXX XXX
			 *
			 * This action is useful for proper "timing" of certain
			 * visual effects, such as breath attacks.
			 *
			 * This action is optional, but may be required by this file,
			 * especially if special "macro sequences" must be supported.
			 */

			sceKernelDelayThread(v * 1000);
						
			return (0);
		}
	}

	/* Unknown or Unhandled action */
	return (1);
}


/*
 * Display the cursor
 *
 * This routine should display the cursor at the given location
 * (x,y) in some manner.  On the PSP it involves drawing a fake
 * cursor.  Note the "soft_cursor" flag which tells "z-term.c" to 
 * treat the "cursor" as a "visual" thing and not as a "hardware" 
 * cursor.
 *
 * You may assume "valid" input if the window is properly sized.
 *
 * You may use the "Term_what(x, y, &a, &c)" function, if needed,
 * to determine what attr/char should be "under" the new cursor,
 * for "inverting" purposes or whatever.
 */
static errr Term_curs_psp(int x, int y)
{
	/* Cursor is enabled until this location is written again by Term_text_psp */
	curs_enable = 1;

	/* Record the current cursor state */
	curs_x = x;
	curs_y = y;
	curs_on = 1;
	Term_what(x, y, &curs_back_attr, &curs_back_char);
				
	/* Draw a solid block */
	psp_put_char(x * CHAR_WIDTH, y * CHAR_HEIGHT, 0xFFFFFF, 0x000000, BLOCK_CHAR); 

	/* Success */
	return (0);
}

/*
 * Helper function that updates the cursor and flips its state
 */
void psp_cursor_flip() {
	if (curs_enable) {
		if (curs_on) { /* Either erase the cursor */
			psp_put_char(curs_x * CHAR_WIDTH, curs_y * CHAR_HEIGHT, 
				pal[curs_back_attr & 0x0F], pal[curs_back_attr >> 4], curs_back_char);
			
			curs_on = 0;
		}
		else { /* Or draw it */
			psp_put_char(curs_x * CHAR_WIDTH, curs_y * CHAR_HEIGHT, 
				0xFFFFFF, 0x000000, BLOCK_CHAR);
			
			curs_on = 1;
		}
	}
}

/*
 * Erase some characters
 *
 * This function should erase "n" characters starting at (x,y).
 *
 * You may assume "valid" input if the window is properly sized.
 *
 * This function works but is not used because always_text is set
 * by Term_data_link below
 */
static errr Term_wipe_psp(int x, int y, int n)
{
  /* Erase one by one (I'm assuming all characters are on one row) */
	int i;
	for (i = 0; i < n; i++) {
		psp_put_char((x + i) * CHAR_WIDTH, y * CHAR_HEIGHT, pal[0], 0x000000, BLOCK_CHAR); 
	}
	
	/* Success */
	return (0);
}


/*
 * Draw some text on the screen
 *
 * This function should actually display an array of characters
 * starting at the given location, using the given "attribute",
 * and using the given string of characters, which contains
 * exactly "n" characters and which is NOT null-terminated.
 *
 * You may assume "valid" input if the window is properly sized.
 *
 * You must be sure that the string, when written, erases anything
 * (including any visual cursor) that used to be where the text is
 * drawn.  
 *
 * In color environments, you should activate the color contained
 * in "color_data[a & 0x0F]", if needed, before drawing anything.
 *
 * Note that if you have changed the "attr_blank" to something
 * which is not black, then this function must be able to draw
 * the resulting "blank" correctly.
 *
 * Note that this function must correctly handle "black" text if
 * the "always_text" flag is set (which it is).  If this flag is not 
 * set, all the "black" text will be handled by the 
 * "Term_wipe_xxx()" hook.
 */
static errr Term_text_psp(int x, int y, int n, byte a, const char *cp)
{

	int i;
	for (i = 0; i < n; i++) {

		/* Disable the cursor if we write over it */
		if (curs_x == x + i && curs_y == y) curs_enable = 0;

		/* Draw the requested text */
		psp_put_char((x + i) * CHAR_WIDTH, y * CHAR_HEIGHT, pal[a & 0x0F], 0x000000, cp[i]);
	}

	/* Success */
	return (0);
}


/*
 * Draw some attr/char pairs on the screen
 *
 * This routine should display the given "n" attr/char pairs at
 * the given location (x,y).  This function is only used if one
 * of the flags "always_pict" or "higher_pict" is defined.
 *
 * You must be sure that the attr/char pairs, when displayed, will
 * erase anything (including any visual cursor) that used to be at
 * the given location.  On many machines this is automatic, but on
 * others, you must first call "Term_wipe_psp(x, y, 1)".
 *
 * With the "higher_pict" flag, this function can be used to allow
 * the display of "pseudo-graphic" pictures, for example, by using
 * the attr/char pair as an encoded index into a pixmap of special
 * "pictures".
 *
 * With the "always_pict" flag, this function can be used to force
 * every attr/char pair to be drawn by this function, which can be
 * very useful if this file can optimize its own display calls.
 *
 * This function is often associated with the "arg_graphics" flag.
 *
 * This function is only used if one of the "higher_pict" and/or
 * "always_pict" flags are set.
 */
static errr Term_pict_psp(int x, int y, int n, const byte *ap, const char *cp,
                          const byte *tap, const char *tcp)
{
	/* Ignore because higher_pict and always_pict are off */

	/* Success */
	return (0);
}



/*** Internal Functions ***/


/*
 * Instantiate a "term_data" structure
 *
 * This is one way to prepare the "term_data" structures and to
 * "link" the various informational pieces together.
 *
 * This function assumes that every window should be ROWSxCOLS in size
 * (the standard size) and should be able to queue 256 characters.
 * Technically, only the "main screen window" needs to queue any
 * characters, but this method is simple.  One way to allow some
 * variation is to add fields to the "term_data" structure listing
 * parameters for that window, initialize them in the "init_xxx()"
 * function, and then use them in the code below.
 *
 * Note that "activation" calls the "Term_init_xxx()" hook for
 * the "term" structure, if needed.
 */
static void term_data_link(int i)
{
	term_data *td = &data[i];
	term *t = &td->t;

	/* Initialize the term */
	term_init(t, ANGBAND_SCREEN_COLS, ANGBAND_SCREEN_ROWS, 256);

	/* Choose "soft" or "hard" cursor XXX XXX XXX */
	/* A "soft" cursor must be explicitly "drawn" by the program */
	/* while a "hard" cursor has some "physical" existance and is */
	/* moved whenever text is drawn on the screen.  See "z-term.c". */
	 t->soft_cursor = TRUE; 

	/* Avoid the "corner" of the window XXX XXX XXX */
	/* t->icky_corner = TRUE; */

	/* Use "Term_pict()" for all attr/char pairs XXX XXX XXX */
	/* See the "Term_pict_xxx()" function above. */
	/* t->always_pict = TRUE; */

	/* Use "Term_pict()" for some attr/char pairs XXX XXX XXX */
	/* See the "Term_pict_xxx()" function above. */
	/* t->higher_pict = TRUE; */

	/* Use "Term_text()" even for "black" text XXX XXX XXX */
	/* See the "Term_text_xxx()" function above. */
	t->always_text = TRUE; 

	/* Ignore the "TERM_XTRA_BORED" action XXX XXX XXX */
	/* This may make things slightly more efficient. */
	t->never_bored = TRUE; 

	/* Ignore the "TERM_XTRA_FROSH" action XXX XXX XXX */
	/* This may make things slightly more efficient. */
	t->never_frosh = TRUE; 

	/* Erase with "white space" XXX XXX XXX */
	t->attr_blank = TERM_DARK; 
	t->char_blank = ' '; 

	/* Prepare the init/nuke hooks */
	t->init_hook = Term_init_psp;
	t->nuke_hook = Term_nuke_psp;

	/* Prepare the template hooks */
	t->user_hook = Term_user_psp;
	t->xtra_hook = Term_xtra_psp;
	t->curs_hook = Term_curs_psp;
	t->wipe_hook = Term_wipe_psp;
	t->text_hook = Term_text_psp;
	t->pict_hook = Term_pict_psp;

	/* Remember where we came from */
	t->data = td;

	/* Activate it */
	Term_activate(t);

	/* Global pointer */
	angband_term[i] = t;
}


/*
 * Writes n characters starting from x,y to the screen based on what 
 * Term_what says is supposed to be on the screen.  Used for refreshing
 */
void psp_refresh(int x, int y, int n) {
	int i;
	unsigned char a;
	char c;

	for (i = 0; i < n; i++) {
		/* If Angband doesn't draw to this part of the screen, draw black */
		if (y >= ANGBAND_SCREEN_ROWS || x + i >= ANGBAND_SCREEN_COLS) 
			psp_put_char((x + i) * CHAR_WIDTH, y * CHAR_HEIGHT, 0, 0, ' ');
		/* Otherwise, see what's in Angband's buffer */
		else {
			Term_what(x + i, y, &a, &c);
		
			psp_put_char((x + i) * CHAR_WIDTH, y * CHAR_HEIGHT, 
				pal[a & 0x0F], pal[(a & 0xF0) >> 4], c);
		}
	}	
}


#ifdef INTERNAL_MAIN


/*
 * Some special machines need their own "main()" function, which they
 * can provide here, making sure NOT to compile the "main.c" file.
 *
 * These systems usually have some form of "event loop", run forever
 * as the last step of "main()", which handles things like menus and
 * window movement, and calls "play_game(FALSE)" to load a game after
 * initializing "savefile" to a filename, or "play_game(TRUE)" to make
 * a new game.  The event loop would also be triggered by "Term_xtra()"
 * (the TERM_XTRA_EVENT action), in which case the event loop would not
 * actually "loop", but would run once and return.
 */


/*
 * Init some stuff
 *
 * This function is used to keep the "path" variable off the stack.
 */
static void init_stuff(void)
{
	char path[1024];
	char* p;
	/* setup plog aux */
	plog_aux = psp_plog;

	/* copy arv0 to path  */
	strlcpy(path,argv0,1024);

	/* Find last '/' */ 
	p = strrchr(path, '/');

	/* Can't happen */
	if (p == NULL) 
	{
		quit("Couldn't get path from argv0");
		return;
	}

	/* add lib path in place of eboot and terminate */
	strcpy(p, "/lib");

	/* Hack -- Add a path separator (only if needed) */
	if (!suffix(path, PATH_SEP)) my_strcat(path, PATH_SEP, sizeof(path));

	/* Initialize */
	init_file_paths(path);
	
	
}


/*
 * Prepare the screen for drawing (480 x 272 x 24)
 */
void psp_screen_init() {
	vram_base = (void*) (0x40000000 | (unsigned long)sceGeEdramGetAddr());
	sceDisplaySetMode(0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);
	sceDisplaySetFrameBuf((void*) vram_base, PSP_LINE_SIZE, PSP_PIXEL_FORMAT, 1);
	screen_init = 1;
}

/*
 * Put a character with the given color and background color at (row,col)
 */

void psp_put_char_aligned(int col, int row, unsigned long color, unsigned long bk_color, unsigned char ch){
	psp_put_char(col * CHAR_WIDTH, row * CHAR_HEIGHT, color, bk_color, ch);
}
/*
 * Put a character with the given color and background color at (x,y)
 */
void psp_put_char(int x, int y, unsigned long color, unsigned long bk_color, unsigned char ch) {
	if (!screen_init) return;

	unsigned long* vram = vram_base + y * PSP_LINE_SIZE + x;
	unsigned char* font_ptr = &font[(int)ch * 8];
	
	int i, j;
	for (i = 0; i < 8; i++, font_ptr++) {
		unsigned long* vram_ptr = vram;
		for (j = 0; j < 6; j++) {
			if ((*font_ptr & (128 >> j))) 
				*vram_ptr++ = color;
			else
				*vram_ptr++ = bk_color;
		}
		vram += PSP_LINE_SIZE;
	}
}

void psp_put_string_aligned(int col, int row, unsigned long color, unsigned long bk_color, char* string) {
	psp_put_string(col * CHAR_WIDTH, row * CHAR_HEIGHT, color, bk_color, string);
}
/*
 * Write a string with the given color and background color starting from (x,y)
 */
void psp_put_string(int x, int y, unsigned long color, unsigned long bk_color, char* string) {
	int i;
	for (i = 0; i < strlen(string); i++) {
		psp_put_char(x + CHAR_WIDTH * i, y, color, bk_color, string[i]);
	}
}

// These two color functions are basically taken from easyrgb.com
float h2rgb(float v1, float v2, float vH) {
	while (vH < 0) vH += 1;
	while (vH > 1) vH -= 1;
	if (6 * vH < 1) return (v1 + (v2 - v1) * 6 * vH);
	if (2 * vH < 1) return (v2);
	if (3 * vH < 2) return (v1 + (v2 - v1) * (((float)2 / 3) - vH) * 6);
	return v1;
}


/*
 * Converts a (hue, saturation, luminance) value to a bgr value we can 
 * write to the screen.  Convenient for shifting through the color spectrum
 */
unsigned long hsl2bgr(float h, float s, float l) {
	// return the luminance if unsaturated
	if (s == 0) return ((unsigned long)((unsigned char)(l * 255))) * 0x010101;

	float var2;
	if (l < 0.5) var2 = l * (1 + s);
	else         var2 = (l + s) - (s * l);

	float var1 = 2 * l - var2;

	unsigned char r = 255 * h2rgb(var1, var2, h + ((float)1/3));
	unsigned char g = 255 * h2rgb(var1, var2, h);
	unsigned char b = 255 * h2rgb(var1, var2, h - ((float)1/3));

	return r + (g << 8) + (b << 16);
}



/*
 * Maps the analog stick coordinates from a square (not a circle
 * as one might presume from the way the stick actually moves) to 
 * one of the 8 cardinal/intercardinal directions or centered (STICK_C). 
 * The mapping is done with a simple 3x3 tic-tac-toe grid.  Although
 * it would seem difficult to go from centered to a diagonal without
 * passing through one of the cardinal zones, in practice it's not 
 * a problem
 */
int psp_analog_octant(int x, int y) {
	if (x < 83) { // left
		if (y < 83) return STICK_NW;
		if (y < 167) return STICK_W;
		return STICK_SW;
	}
	else if (x < 167) { // center
		if (y < 83) return STICK_N;
		if (y < 167) return STICK_C;
		return STICK_S;
	}
	else { // right
		if (y < 83) return STICK_NE;
		if (y < 167) return STICK_E;
		return STICK_SE;
	}
}


/*
 * Updates the IO state variables according to what buttons are currently
 * pressed and what position the analog stick is in
 */
void psp_update_keys() {
	SceCtrlData pad;
	sceCtrlReadBufferPositive(&pad, 1); 

	int i;
	for (i = 0; i < NUM_KEYS; i++) { 
		/* If a button is pressed, increment its counter */
		if (pad.Buttons & key_signals[i]) key_state[i]++; 

		/* If it's not pressed, reset its counter */
		else key_state[i] = -1; 
	} 

	int new_octant = psp_analog_octant(pad.Lx, pad.Ly); 
	if (new_octant == analog_octant) analog_state++; 
	else analog_state = -3; // -3 gives some hysteresis of sorts between octants

	analog_octant = new_octant; 				
}


void psp_draw_menu_border(int x, int y,int rows, int cols) {
	int i, j;
	
	
	//draw border
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			psp_put_char_aligned(x-1 + j,
				y-1 + i,
				0xFF0000, 0x000000, BLOCK_CHAR);
		}
	}
	//erase inside with black
	for (i = 0; i < rows-2; i++) {
		for (j = 0; j < cols-2; j++) {
			psp_put_char_aligned(x + j,
				y + i,
				0x000000, 0x000000, BLOCK_CHAR);
		}
	}

}
/*
 * Draws the onscreen keyboard
 */
void psp_show_osk() {
	
	psp_draw_menu_border(KEYBOARD_COL,KEYBOARD_ROW,8,15);
	/* Draw the individual letters */
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 13; j++) {
			psp_put_char_aligned(KEYBOARD_COL + j, 
											KEYBOARD_ROW + (i+1), 
											0xFFFFFF, 0x000000, 
											keys[HELD(KEY_RTRIGGER) ? 1 : 0][j + i * 13]);
		}
	}

	/* Draw the weird sized keys */
	psp_put_string_aligned(KEYBOARD_COL + 1,     KEYBOARD_ROW,
									0xFFFFFF, 0x000000, " Esc ");
	psp_put_string_aligned(KEYBOARD_COL + 7, KEYBOARD_ROW,                   
									0xFFFFFF, 0x000000, "Bkspc");
	psp_put_string_aligned(KEYBOARD_COL + 1,     KEYBOARD_ROW + 5, 
									0xFFFFFF, 0x000000, "Space");
	psp_put_string_aligned(KEYBOARD_COL + 7, KEYBOARD_ROW + 5, 
									0xFFFFFF, 0x000000, "Enter");
}


/*
 * Highlight the keyboard button at (cx,cy) with color
 */
void psp_highlight_osk(int cx, int cy, int color) {
	/* Top row -> ESC or BKSPC */
	if (!cy) {
		if (cx < 6) 
			psp_put_string_aligned(KEYBOARD_COL + 1,
											KEYBOARD_ROW, 0xFFFFFF, color, " Esc ");
		else 
			psp_put_string_aligned(KEYBOARD_COL + 7,
											KEYBOARD_ROW, 0xFFFFFF, color, "Bkspc");	
	}
	/* Middle rows */
	else if (cy < 5) {
		psp_put_char_aligned(KEYBOARD_COL + cx, 
									KEYBOARD_ROW + cy,
									0xFFFFFF, color, 
									keys[HELD(KEY_RTRIGGER) ? 1 : 0][cx + (cy-1) * 13]);
	}
	/* Bottom row -> SPACE or ENTER */
	else {
		if (cx < 6) 
			psp_put_string_aligned(KEYBOARD_COL + 1, 
											KEYBOARD_ROW + 5, 
											0xFFFFFF, color, "Space");
		else 
			psp_put_string_aligned(KEYBOARD_COL + 7, 
											KEYBOARD_ROW + 5, 
											0xFFFFFF, color, "Enter");
	}
}


/*
 * Erase the keyboard by putting back whatever characters Term_what says 
 * are supposed to be there.
 */
void psp_hide_osk() {
	/* Keyboard is now 8x15 with borders and aligned */
	Term_redraw_section(KEYBOARD_COL-1,KEYBOARD_ROW-1,KEYBOARD_COL+15,KEYBOARD_ROW+8);

}

/*
 * Bring up the onscreen keyboard and return the selected key.  Returns 0
 * if no key is selected (user cancelled with circle or chose invalid key)
 */
char psp_do_osk() {
	static int cx = 6; // static so we remember the last cursor position
	static int cy = 3;

	psp_show_osk();
			
	while (HELD(KEY_TRIANGLE)) { // Maintain keyboard until triangle released
					
		/* Remember the old button states */
		int oldrt = HELD(KEY_RTRIGGER);
		int oldcx = cx;
		int oldcy = cy;
		
		/* Get the new button states */
		psp_update_keys();		

		/* Highlight the selected whatever */
		psp_highlight_osk(cx, cy, 0xA05000);
		
		/* Evaluate keys and respond */
		if (cy > 0 && cy < 5) { /* Middle rows change X by one */
			if (PRESSED(KEY_LEFT)  && --cx < 0)  cx = 12;
			if (PRESSED(KEY_RIGHT) && ++cx > 12) cx = 0;
		}
		else { /* Top and bottom rows switch between left and right side */
			if (PRESSED(KEY_LEFT) || PRESSED(KEY_RIGHT)) { 
				if (cx < 6) cx += 6;
				else cx -= 6;
			}
		}
		if (PRESSED(KEY_UP)    && --cy < 0)  cy = 5;
		if (PRESSED(KEY_DOWN)  && ++cy > 5)  cy = 0;
		
		/* Cancel if circle pressed */
		if (PRESSED(KEY_CIRCLE)) {
			psp_hide_osk(); /* give user visual feedback that OSK was cancelled */
			/* Wait until Triangle is released so we don't end up calling 
			 * psp_do_osk again */
			while (HELD(KEY_TRIANGLE)) psp_update_keys();
			
			return 0; // and then exit with no key
		}
			
		/* If the trigger state changed, draw the new letter set */
		if (HELD(KEY_RTRIGGER) != oldrt) psp_show_osk();
		
		/* If something changed, erase the old highlight */	
		if (cx != oldcx || cy != oldcy || HELD(KEY_RTRIGGER) != oldrt)
			psp_highlight_osk(oldcx, oldcy, 0x000000);

	}
			
	psp_hide_osk();

	/* Return the selected key */
	if (!cy) {
		if (cx < 6) return 0x1B; // esc
		else return 0x08; // backspace
	}
	else if (cy < 5) {
		/* Return the selected letter from the keys array, accounting for Shift */
		char ascii_value = keys[HELD(KEY_RTRIGGER) ? 1 : 0][cx + (cy-1) * 13];

		/* Attempt to send ctrl modifier if left trigger held */
		if (HELD(KEY_LTRIGGER)) {
			char ctrl_ascii_value = KTRL(ascii_value);
			/* But only if a valid input results */
			if (ctrl_ascii_value >= KTRL('A') && ctrl_ascii_value <= KTRL('Z'))
				return KTRL(ascii_value); 
			else
				return 0;
		}
		else return ascii_value;
	}
	else {
		if (cx < 6) return ' '; // space
		else return 0x0D; // enter
	}
}


/* 
 * Draws the macro interface
 */
void psp_show_star(star_t* star) {
	/* Only draw if there are children stars to draw too */
	if (star->links[0]) {
		char text[14];

		/* Render the 5 options.  Limit to 13 chars or bad things happen */
		snprintf(text, 13, "\x19:%s", star->links[0]->desc);
		psp_put_string(60, 264, 0xCC66FF, 0x000000, text);

		snprintf(text, 13, "\x18:%s", star->links[1]->desc);
		psp_put_string(144, 264, 0x66FFFF, 0x000000, text);
		
		/* 
		 * Special fix for the center option to keep the ] from disappearing 
		 * if the description is too long 
		 */
		snprintf(text, 12, "[%s", star->desc);
		strcat(text, "]"); 
		psp_put_string(228, 264, 0x66FF33, 0x000000, text);
		
		snprintf(text, 13, "\x1A:%s", star->links[3]->desc);
		psp_put_string(312, 264, 0xFF6633, 0x000000, text);

		snprintf(text, 13, "\x1B:%s", star->links[2]->desc);
		psp_put_string(396, 264, 0xFF33CC, 0x000000, text);
	}
}


/* 
 * Erases the macro interface
 */
void psp_hide_star() {
	psp_put_string(60, 264, 0x000000, 0x000000, "                                                                      ");
}


/*
 * Write a macro star to disk, then recursively write its children too
 */
void psp_save_star(int fff, star_t* star) {
				
	/* Write both strings with their null terminator (as a separator) */
	sceIoWrite(fff, &star->macro, strlen(&star->macro[0]) + 1);
	sceIoWrite(fff, &star->desc, strlen(&star->desc[0]) + 1);
				
	if (star->links[0] != NULL) {
		psp_save_star(fff, star->links[0]);
		psp_save_star(fff, star->links[1]);
		psp_save_star(fff, star->links[2]);
		psp_save_star(fff, star->links[3]);
	}
}


/* 
 * Write all macro stars to disk
 */
void psp_save_macros() {
	char buf[1024];
	sprintf(buf,"%smacros.txt",ANGBAND_DIR);
	int fff = sceIoOpen(buf, PSP_O_WRONLY | PSP_O_TRUNC | PSP_O_CREAT, 0777);
	psp_save_star(fff, &star);
	sceIoClose(fff);
}


/*
 * Load a macro star from disk, then load its children too
 */
void psp_load_star(int fff, star_t* star) {

	char c;				

	/* Read until the first null; that's the macro */
	char* text_ptr = &star->macro[0];
	do {
		sceIoRead(fff, &c, 1);
		*text_ptr++ = c;
	} while (c != 0);	

	/* Read until the next null; that's the description */
	text_ptr = &star->desc[0];
	do {
		sceIoRead(fff, &c, 1);
		*text_ptr++ = c;
	} while (c != 0);	
	
	if (star->links[0] != NULL) {
		psp_load_star(fff, star->links[0]);
		psp_load_star(fff, star->links[1]);
		psp_load_star(fff, star->links[2]);
		psp_load_star(fff, star->links[3]);
	}
}


/*
 * Load all macro stars from disk
 */
void psp_load_macros() {
	char buf[1024];
	sprintf(buf,"%smacros.txt",ANGBAND_DIR);
	int fff = sceIoOpen(buf, PSP_O_RDONLY, 0777);

	/* If the file was not there, we'll use the defaults */
	if (fff < 0) 
		return;
	
	psp_load_star(fff, &star);
	sceIoClose(fff);				
}


/*
 * Walks the user through the process of changing one of the macros
 */
void psp_modify_macro() {
	static char cmd1[] = "D-pad: Select macro to replace \x12\x13: Finish \x10\x11: Cancel";
	static char cmd2[] = "\x14\x15: Type macro \x12\x13: Finish \x10\x11: Cancel";
	static char cmd3[] = "\x14\x15: Type description (11 chars or less) \x12\x13: Finish \x10\x11: Cancel";
	static char cmd4[] = "Saving change...";
	
	/***** Stage 1: Choose the macro to replace *****/
	
	/* Print instructions */
	psp_put_string(240 - 3 * strlen(cmd1), INSTR_ROW * CHAR_HEIGHT, 0x606060, 0x000000, cmd1);	
	
	/* Start with the topmost star */
	star_t* c_star = &star;
				
	/* Loop until a macro is chosen, either by reaching a 'leaf' macro or 
	 * by breaking when Cross is pressed */
	while (c_star->links[0] != NULL) {
		psp_show_star(c_star);

		psp_update_keys();

		if (PRESSED(KEY_CROSS)) // Select -> Go on
			break;
		else if (PRESSED(KEY_CIRCLE)) { // Cancel -> Erase interface and return
			psp_refresh(0, INSTR_ROW, 80);
			psp_hide_star();
			return;
		}
		else if (PRESSED(KEY_LEFT))  {c_star = c_star->links[0]; psp_hide_star();}
		else if (PRESSED(KEY_UP))    {c_star = c_star->links[1]; psp_hide_star();}
		else if (PRESSED(KEY_RIGHT)) {c_star = c_star->links[2]; psp_hide_star();}
		else if (PRESSED(KEY_DOWN))  {c_star = c_star->links[3]; psp_hide_star();}
	}

	/* Cleanup from stage 1 */
	psp_refresh(0, INSTR_ROW, 80);
	psp_hide_star();
	
	/* Wait for release so one press doesn't count as two */
	while (HELD(KEY_CROSS)) psp_update_keys();
	
	
	/***** Stage 2: Enter the macro *****/	

	/* Print instructions */
	psp_put_string(240 - 3 * strlen(cmd2), INSTR_ROW * CHAR_HEIGHT, 0xA0A0A0, 0x000000, cmd2);

	char macro[64];
	unsigned char macro_idx = 0;
	
	/* Accept OSK keys until out of space or cross pressed */
	while (!HELD(KEY_CROSS) && macro_idx < 63) {
		psp_update_keys();

		if (HELD(KEY_TRIANGLE)) {
			/* Get a key */
			char c = psp_do_osk();

			/* Handle backspace by removing character and redrawing */
			if (c == 0x08) {
				if (macro_idx > 0) {
					macro_idx--;
					macro[macro_idx] = 0;
					psp_refresh(0, 33, 80);
					psp_put_string(240 - 3 * strlen(macro), 264, 0xA0A0A0, 0x000000, macro);
				}
			}
			/* Handle regular chars */
			else if (c) {
				macro[macro_idx++] = c;
				macro[macro_idx] = 0;
				psp_put_string(240 - 3 * strlen(macro), 264, 0xA0A0A0, 0x000000, macro);
			}
		}
		else if (HELD(KEY_CIRCLE)) { // Cancel -> Erase interface and return
			psp_refresh(0, INSTR_ROW, 80);
			psp_refresh(0, 33, 80);
			return;
		}
	}

	/* Cleanup from stage 2 */
	psp_refresh(0, INSTR_ROW, 80);
	psp_refresh(0, 33, 80);	
	macro[macro_idx] = 0;

	/* Wait for release so one press doesn't count as two */
	while (HELD(KEY_CROSS)) psp_update_keys();

	
	/***** Stage 3: Enter the description *****/		
	
	/* Print instructions */
	psp_put_string(240 - 3 * strlen(cmd3), INSTR_ROW * CHAR_HEIGHT, 0xA0A0A0, 0x000000, cmd3);
	
	char desc[12];
	unsigned char desc_idx = 0;
	
	/* Accept OSK keys until out of space or cross pressed */
	while (!HELD(KEY_CROSS) && desc_idx < 11) {
		psp_update_keys();

		if (HELD(KEY_TRIANGLE)) {
			char c = psp_do_osk();

			/* Handle backspace by removing character and redrawing */
			if (c == 0x08) {
				if (desc_idx > 0) {
					desc_idx--;
					desc[desc_idx] = 0;
					psp_refresh(0, 33, 80);
					psp_put_string(240 - 3 * strlen(desc), 264, 0xA0A0A0, 0x000000, desc);
				}
			}
			/* Handle regular chars */
			else if (c) {
				desc[desc_idx++] = c;
				desc[desc_idx] = 0;
				psp_put_string(240 - 3 * strlen(desc), 264, 0xA0A0A0, 0x000000, desc);
			}
		}
		else if (HELD(KEY_CIRCLE)) { // Cancel -> Erase interface and return
			psp_refresh(0, INSTR_ROW, 80);
			psp_refresh(0, 33, 80);
			return;
		}
	}

	/* Cleanup from stage 3 */
	psp_refresh(0, INSTR_ROW, 80);
	psp_refresh(0, 33, 80);
	desc[desc_idx] = 0;	

	
	/***** Stage 4: Save *****/	
	
	/* Print notification */
	psp_put_string(240 - 3 * strlen(cmd4), INSTR_ROW * CHAR_HEIGHT, 0xA0A0A0, 0x000000, cmd4);

	/* Update the star chosen in stage 1 with the data from stages 2 and 3 */
	strcpy(c_star->macro, macro);
	strcpy(c_star->desc, desc);
	psp_save_macros();

	/* Cleanup from stage 4 */
	psp_refresh(0, INSTR_ROW, 80);
}


/*
 * Bring up the macro interface and execute the selected macro
 */
void psp_do_macro() {

	/* Start with the topmost star */
	star_t* c_star = &star;
				
	/* Loop until a macro is chosen, either by reaching a 'leaf' macro or 
	 * by breaking when Cross is pressed */	
	while (HELD(KEY_SQUARE) && c_star->links[0] != NULL) {
		psp_show_star(c_star);

		psp_update_keys();

		if (PRESSED(KEY_CROSS)) { // Change macro
			psp_hide_star();
			psp_modify_macro();
			return;
		}
		else if (PRESSED(KEY_CIRCLE)) { // Cancel
			while (HELD(KEY_SQUARE)) psp_update_keys();
			psp_hide_star();
			return;
		}
		else if (PRESSED(KEY_LEFT))  {c_star = c_star->links[0]; psp_hide_star();}
		else if (PRESSED(KEY_UP))    {c_star = c_star->links[1]; psp_hide_star();}
		else if (PRESSED(KEY_RIGHT)) {c_star = c_star->links[2]; psp_hide_star();}
		else if (PRESSED(KEY_DOWN))  {c_star = c_star->links[3]; psp_hide_star();}
	}

	psp_hide_star();

	/* Issue the commands in the selected macro */
	int i;
	/* escape 3 times to clear command buffer */
	Term_keypress('\e');
	Term_keypress('\e');
	Term_keypress('\e');
	for (i = 0; i < strlen(c_star->macro); i++) {
		Term_keypress(c_star->macro[i]);
	}	

	/* Wait for square to be released before proceeding */
	while (HELD(KEY_SQUARE)) psp_update_keys();
}

/*
 * Called when the user tries to exit via the HOME button
 */
int psp_exit_callback(void)
{	
	sceKernelExitGame();
	return 0;
}


/*
 * Thread for responding to exit and power callbacks
 */
void psp_callback_thread(void *arg)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", psp_exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	
	sceKernelSleepThreadCB();
}

/*
 * Timer thread
 *
 * Blinks the cursor at 3 Hz.  Also used to update the battery meter
 */
void psp_timer_thread(void* arg)
{
				
	while (1) {
		/* Update cursor */
		psp_cursor_flip();
		
		/* Update battery meter */
		if (scePowerIsBatteryExist()) {
			char battery_text[10] = "[";
			
			/* If plugged in, state such */
			if (scePowerIsPowerOnline())
				strcat(battery_text, "AC/");

			char charge = scePowerGetBatteryLifePercent();
			sprintf(&battery_text[strlen(battery_text)], "%d%%", charge);

			strcat(battery_text, "]");

			/* Pad the text to fill 9 spaces to cleanup previous meter */
			while (strlen(battery_text) < 9) strcat(battery_text, " ");

			/* 
			 * Print the string we've built using a color that shifts 
			 * from blue to red as battery power falls.
			 */
			psp_put_string(0, 264, 
					hsl2bgr((float)2/3 * (charge)/100, 0.8, 0.6),
					0x000000, battery_text); 
		}
		else {
			/* No battery, so just write [AC] in white */
			psp_put_string(0, 264, 0xFFFFFF, 0x000000, "[AC]     ");
		}

		/* Interrupt at 6 Hz -> cursor blinks at 3 Hz */
		sceKernelDelayThread(166666);
	}
}


/*
 * Input thread
 *
 * Sends Angband Term_keypress calls depending on PSP buttons that are pressed
 */

void psp_input_thread(void* arg)
{
				
	int i;
	for (i = 0; i < NUM_KEYS; i++) key_state[i] = -1;

	/* At startup, try to load a saved macro file */
	psp_load_macros();
	
	while (1) {
		psp_update_keys();

		if (HELD(KEY_TRIANGLE)) {
			char c = psp_do_osk();
			if (c) Term_keypress(c);
		}
		else if (HELD(KEY_SQUARE)) psp_do_macro();
		else {
			if (PRESSED(KEY_DOWN) || A_PRESSED(STICK_S))  
				{TRY_RUN; Term_keypress(0x32);} // down
			if (PRESSED(KEY_LEFT) || A_PRESSED(STICK_W)) 
				{TRY_RUN; Term_keypress(0x34);} // left
			if (PRESSED(KEY_UP)   || A_PRESSED(STICK_N)) 
				{TRY_RUN; Term_keypress(0x38);} // up
			if (PRESSED(KEY_RIGHT)|| A_PRESSED(STICK_E)) 
				{TRY_RUN; Term_keypress(0x36);} // right	
			if (A_PRESSED(STICK_NW)) 
				{TRY_RUN; Term_keypress(0x37);}
			if (A_PRESSED(STICK_NE))
				{TRY_RUN; Term_keypress(0x39);}
			if (A_PRESSED(STICK_SW))
				{TRY_RUN; Term_keypress(0x31);}
			if (A_PRESSED(STICK_SE))
				{TRY_RUN; Term_keypress(0x33);}
			if (PRESSED(KEY_CIRCLE)) Term_keypress(0x1B); // esc
			if (PRESSED(KEY_CROSS)) Term_keypress(0x0D); // enter
			if (PRESSED(KEY_SELECT)) Term_keypress('*'); // find target
			if (PRESSED(KEY_START)) Term_keypress('t'); // select target
		}
		
		sceDisplayWaitVblankStart();
//		sceKernelDelayThread(20000); // Sleep for 20ms
	}
}

/*
 * A hook for "quit()".
 *
 * Close down, then fall back into "quit()".
 */
static void quit_hook(cptr s)
{
	int j;

	/* plog out s since quit() isn't used */
	plog(s);

	/* Scan windows */
	for (j = ANGBAND_TERM_MAX - 1; j >= 0; j--)
	{
		/* Unused */
		if (!angband_term[j]) continue;

		/* Nuke it */
		term_nuke(angband_term[j]);
	}


	/* Kill off the threads */
	if (cb_thid >= 0) sceKernelTerminateThread(cb_thid);
	if (input_thid >= 0) sceKernelTerminateThread(input_thid);
	if (timer_thid >= 0) sceKernelTerminateThread(timer_thid);

	if (s != NULL)
	{	
		/* wait for user to press x to view exit message */
		psp_put_string_aligned(33,25, 0xCC66FF, 0x000000,"Hit \x12\x13 to exit");
		while (!HELD(KEY_CROSS)){psp_update_keys();}
	}
	
	sceKernelExitGame();

}

/*
 * Main thread
 *
 * Initializes and executes Angband
 */
void psp_main_thread(void* arg)
{
	
	/* Get the file paths */
	init_stuff();

	/* Process the player name */
	process_player_name(TRUE);

	/* Bypass the modules[] stuff from main.c */
	term_data_link(0);
	ANGBAND_SYS = "psp";
				
	/* Initialize */
	init_angband();

	/* Wait for response */
	pause_line(Term->hgt - 3);

	/* Play the game */
	play_game(FALSE);

	/* Free resources */
	cleanup_angband();

	sceKernelExitGame();
	

}

/* 
 * Entry point
 *
 * Do PSP specific initialization, then create the four threads that 
 * comprise the game.
 */
int main(int argc, char *argv[])
{

	/* Save the "program name" XXX XXX XXX */
	argv0 = argv[0];
	
	
	psp_screen_init();
	pspDebugScreenInit();
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	/* Install "quit" hook */
	quit_aux = quit_hook;
	
	cb_thid = sceKernelCreateThread("cback_thread", 
		(SceKernelThreadEntry)psp_callback_thread, 0x11, 0xFA0, 0, 0);
	if (cb_thid >= 0) sceKernelStartThread(cb_thid, 0, 0);

	input_thid = sceKernelCreateThread("input_thread", 
		(SceKernelThreadEntry)psp_input_thread, 0x11, 0xFA0, 0, 0);
	if (input_thid >= 0) sceKernelStartThread(input_thid, 0, 0);
		
	timer_thid = sceKernelCreateThread("timer_thread", 
		(SceKernelThreadEntry)psp_timer_thread, 0x11, 0xFA0, 0, 0);
	if (timer_thid >= 0) sceKernelStartThread(timer_thid, 0, 0);

	main_thid = sceKernelCreateThread("main_thread", 
		(SceKernelThreadEntry)psp_main_thread, 0x12, 0x4000, 0, 0);
	if (main_thid >= 0) sceKernelStartThread(main_thid, 0, 0);

	sceKernelSleepThread();

	return 0;
}

#endif /* INTERNAL_MAIN */

#endif /* USE_PSP */
