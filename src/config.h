/* File: config.h */

/*
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */


/*
 * Look through the following lines, and where a comment includes the
 * tag "OPTION:", examine the associated "#define" statements, and decide
 * whether you wish to keep, comment, or uncomment them.  You should not
 * have to modify any lines not indicated by "OPTION".
 *
 * Note: Also examine the "system" configuration file "h-basic.h".
 *
 * And finally, remember that the "Makefile" will specify some rather
 * important compile time options, like what visual module to use.
 */


/*
 * OPTION: See the Makefile(s), where several options may be declared.
 *
 * Some popular options include "USE_GCU" (allow use with Unix "curses"),
 * "USE_X11" (allow basic use with Unix X11), "USE_XAW" (allow use with
 * Unix X11 plus the Athena Widget set), and "USE_CAP" (allow use with
 * the "termcap" library, or with hard-coded vt100 terminals).
 *
 * Several other such options are available for non-unix machines,
 * such as "MACINTOSH", "WINDOWS", "USE_IBM", "USE_EMX".
 */



/*
 * OPTION: Hack -- Compile in support for "Borg mode"
 */
/* #define ALLOW_BORG */


/*
 * OPTION: Hack -- Compile in support for "Debug Commands"
 */
#define ALLOW_DEBUG

/*
 * OPTION: Hack -- Compile in support for "Spoiler Generation"
 */
#define ALLOW_SPOILERS

/*
 * OPTION: Allow "do_cmd_colors" at run-time
 */
#define ALLOW_COLORS

/*
 * OPTION: Allow "do_cmd_visuals" at run-time
 */
#define ALLOW_VISUALS

/*
 * OPTION: Allow "do_cmd_macros" at run-time
 */
#define ALLOW_MACROS


/*
 * OPTION: Allow parsing of the ascii template files in "init.c".
 * This must be defined if you do not have valid binary image files.
 * It should be usually be defined anyway to allow easy "updating".
 */
#define ALLOW_TEMPLATES


/*
 * OPTION: Allow processing of template files once parsed.
 * This 'evaluates' the contents of the files. It is is currently
 * used for balancing the monster list (monster.txt).
 */

/* #define ALLOW_TEMPLATES_PROCESS */


/*
 * OPTION: Allow output of 'parsable' ascii template files.
 * This can be used to help change the ascii template format, and to
 * make changes to the data in the parsed files within Angband itself.
 *
 * Files are output to lib\user with the same file names as lib\edit.
 */

/* #define ALLOW_TEMPLATES_OUTPUT */



/*
 * OPTION: Allow "Borgs" to yield "high scores"
 */
/* #define SCORE_BORGS */



/*
 * OPTION: Allow use of the "adult_ai_smell" and "adult_ai_sound"
 * software options, which enable "monster flowing".
 */
#define MONSTER_FLOW



/*
 * OPTION: Allow use of the "adult_ai_smart" and "adult_ai_packs"
 * software options, which attempt to make monsters smarter.
 *
 * AI code by Keldon Jones (keldon@umr.edu), modified by Julian
 * Lighton (jl8e@fragment.com).
 */
#define MONSTER_AI


/*
 * OPTION: Use the "complex" wall illumination code
 */
/* #define UPDATE_VIEW_COMPLEX_WALL_ILLUMINATION */


/*
 * OPTION: Gamma correct colours (with X11)
 */
#define SUPPORT_GAMMA


/*
 * OPTION: Check the modification time of *_info.raw files
 */
#define CHECK_MODIFICATION_TIME


/*
 * OPTION: Enable the "adult_ai_learn" and "adult_ai_cheat" options.
 *
 * They let monsters make more "intelligent" choices about attacks (including
 * spell attacks) based on their observations of the player's reactions to
 * previous attacks.  The "cheat" option lets the monster know how the player
 * would react to an attack without actually needing to make the attack.  The
 * "learn" option requires that a monster make a "failed" attack before
 * learning that the player is not harmed by that attack.
 */
#define DRS_SMART_OPTIONS


/*
 * OPTION: Allow the use of "sound" in various places.
 */
#define USE_SOUND

/*
 * OPTION: Allow the use of "graphics" in various places
 */
#define USE_GRAPHICS




/*
 * OPTION: Set the "default" path to the angband "lib" directory.
 *
 * See "main.c" for usage, and note that this value is only used on
 * certain machines, primarily Unix machines.
 *
 * The configure script overrides this value.  Check the "--prefix=<dir>"
 * option of the configure script.
 *
 * This value will be over-ridden by the "ANGBAND_PATH" environment
 * variable, if that variable is defined and accessable.  The final
 * "slash" is required if the value supplied is in fact a directory.
 *
 * Using the value "./lib/" below tells Angband that, by default,
 * the user will run "angband" from the same directory that contains
 * the "lib" directory.  This is a reasonable (but imperfect) default.
 *
 * If at all possible, you should change this value to refer to the
 * actual location of the "lib" folder, for example, "/tmp/angband/lib/"
 * or "/usr/games/lib/angband/", or "/pkg/angband/lib".
 */
#ifndef DEFAULT_PATH
# define DEFAULT_PATH "./lib/"
#endif /* DEFAULT_PATH */


/*
 * OPTION: Create and use a hidden directory in the users home directory
 * for storing pref-files and character-dumps.
 */
#ifdef SET_UID
# ifndef PRIVATE_USER_PATH
#  define PRIVATE_USER_PATH "~/.angband"
# endif /* PRIVATE_USER_PATH */
#endif /* SET_UID */


/*
 * OPTION: Create and use hidden directories in the users home directory
 * for storing save files, data files, and high-scores
 */
#ifdef PRIVATE_USER_PATH
/* # define USE_PRIVATE_PATHS */
#endif /* PRIVATE_USER_PATH */



/*
 * OPTION: Prevent usage of the "ANGBAND_PATH" environment variable and
 * the '-d<what>=<path>' command line option (except for '-du=<path>').
 *
 * This prevents cheating in multi-user installs as well as possible
 * security problems when running setgid.
 */
#ifdef SET_UID
#define FIXED_PATHS
#endif /* SET_UID */



/*
 * OPTION: Default font (when using X11).
 */
#define DEFAULT_X11_FONT		"9x15"


/*
 * OPTION: Default fonts (when using X11)
 */
#define DEFAULT_X11_FONT_0		"10x20"
#define DEFAULT_X11_FONT_1		"9x15"
#define DEFAULT_X11_FONT_2		"9x15"
#define DEFAULT_X11_FONT_3		"5x8"
#define DEFAULT_X11_FONT_4		"5x8"
#define DEFAULT_X11_FONT_5		"5x8"
#define DEFAULT_X11_FONT_6		"5x8"
#define DEFAULT_X11_FONT_7		"5x8"



/*
 * OPTION: Attempt to minimize the size of the game
 */
/* #define ANGBAND_LITE */


/*
 * Hack -- React to the "ANGBAND_LITE" flag
 */
#ifdef ANGBAND_LITE
# undef ALLOW_COLORS
# undef ALLOW_VISUALS
# undef ALLOW_MACROS
# undef MONSTER_FLOW
# undef DRS_SMART_OPTIONS
# undef ALLOW_BORG
# undef ALLOW_DEBUG
# undef ALLOW_SPOILERS
# undef ALLOW_TEMPLATES
# undef MONSTER_AI
#endif


/*
 * HACK - define if the source contains the cleanup_angband() function.
 */
#define HAS_CLEANUP



/*
 * Allow the Borg to use graphics.
 */
#ifdef ALLOW_BORG
# ifdef USE_GRAPHICS
#  define ALLOW_BORG_GRAPHICS
# endif /* USE_GRAPHICS */
#endif /* ALLOW_BORG */

/*
 * Hack -- PSP stuff
 */
#ifdef USE_PSP

/* Do not handle signals */
# undef HANDLE_SIGNALS
# undef USE_SOUND
# undef USE_GRAPHICS
# define USE_YN_SHORTCUT
#endif