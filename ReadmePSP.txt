Angband 3.0.6 for PSP 0.4:
  by abszero and Gendal
Update to 3.0.9 by Daddy32  


---Setup---
	Copy the 'Angband 3.0.9' folder from the 'GAME' folder
		to the /PSP/GAME directory on your memorystick


---Controls---
	Quick Summary:
		D-Pad:                 Move character (4 way), interface navigation
		Analog Stick:          Move character (8 way)
		Cross:                 Confirm / 'Enter'
		Circle:                Cancel / 'Escape'
		Triangle (held):       Onscreen keyboard (release to select)
		Square (held):         Macro interface
		Square (held) + Cross: Modify macros
		Left Trigger:          CTRL modifier key
		Right Trigger:         Shift modifier key / Run
		Select:                Cycle targets / '*'
		Start:                 Lock on target / 't'
		

	Via Onscreen Keyboard:
		?                      Online help file; 'playing the game' lists commands
		CTRL + x               Save and exit the game
		)                      Save a screenshot


	D-Pad - Move:
		Moves your character up, down, left, or right.  To move diagonally,	which 
		is fairly important in Angband, you would need to use the analog stick 
		described below or the numpad available from the onscreen keyboard (more on
		this later).  On the numpad, directions are as follows:  1 = SW, 2 = S, 
		3 = SE, 4 = W, 6 = E, 7 = NW, 8 = N, and 9 = NE.

	Analog Stick - Alternate Move:
		Moves your character in any of the eight directions.

	Cross - Confirm:
		The confirm button generally.  Maps to the Enter key.  Furthermore, yes/no 
		questions have been modified to treat the cross button as an affirmative 
		response (y and Y are still accepted too).  Many other in-game queries have
		a shortcut response to the Enter key as well.  In stores for example, when 
		you are asked how many of an item you would like, pressing Enter (Cross) 
		will reply with the value '1'.

	Circle - Cancel:
		The cancel button generally.  Maps to the Escape key, thereby cancelling 
		most actions in the game and interface.

	Triangle - Onscreen Keyboard: 
		Brings up the onscreen keyboard (OSK).  As long as you hold the Triangle 
		button, the OSK remains on the screen.  During this time you can move the 
		cursor with the d-pad to select the letter you want.  When you have made 
		your selection, releasing the Triangle button will submit the currently 
		highlighted key to the game.  The OSK remembers the position of the cursor,
		so if you are trying to pick a lock say, you can hold	Triangle, navigate to
		the 'o' key once, release Triangle, and then repeatedly press	Triangle 
		(submitting multiple 'o' presses) until the door opens.  If you have the 
		OSK visible and decide you don't want to enter a key, you can cancel the 
		OSK by pressing Circle.

	Square - Macros:
		Brings up the macro interface.  Unlike the OSK,	the macro interface does 
		not show all the available options at once.  When	you first hold Square, 
		five options appear at the bottom of the screen.  Release the Square button
		will invoke the green option at the center.  If you press an arrow key 
		while	still holding Square, you will instead move to a different 'sub-star'
		according to the arrow symbols on the other 4 options.  

		For	example, in the default macro set, if you hold Square, the option with 
		an up arrow will be a yellow 'Rest HP/MP'.  If you press up while still 
		holding Square, a new	star appears with 'Rest HP/MP' in the center and 
		several other new options surrounding it.  You could now release Square and
		it would select 'Rest HP/MP'.  Alternatively, you	could press another 
		direction to select one of the new sub-stars. Currently there are only two 
		levels of stars: the initial one and its four	sub-stars.  This may change 
		in the future.  

		While this may sound somewhat complicated, with a little practise it is a 
		very efficient way to play the game.  In the default set, for example, you
		can shoot a magic missile at the nearest target just by pressing and 
		releasing Square.  If you want to aim at a different target, hold Square, 
		press Right, then release Square. The star interface puts many options at 
		your fingertips with just a few keypresses.

		What's more, you can modify the set of macros by holding Square to bring
		up the interface and then pressing Cross.  Once you press Cross, you don't 
		need to hold it or Square.  Follow the onscreen directions to make your 
		own macro or press Circle to cancel.  When your macro is finished, it will
		be saved to a file on your memorystick for future use.  When choosing your
		macros, it's a good idea to make the ones you use most often have the
		shortest key sequences.  Also, you might want to organize your macros into
		substars that contain similar macros.  Like in the default set, the 
		up-arrow sub-star is for recovery, left is for detection, down is for
		various dungeon crawling stuff,	and right is for attack magic.

	Left Trigger - CTRL:
		Maps to the CTRL modifier key on a keyboard.  Hold this key down when you
		enter a key via the OSK to enter CTRL + that key instead.  For example, to 
		save and exit (CTRL + X), you would hold Triangle, navigate to 'X', hold 
		the Left Trigger, and release Triangle.  Note that this is not a full 
		implementation of the CTRL button; only 'CTRL + letter' combinations are
		available.

	Right Trigger - Shift:
		Maps to the Shift modifier key on a keyboard.  In the OSK, holding the 
		Right Trigger allows the use of capital letters, etc.  Also, pressing 
		Right Trigger while moving around the Angband map engages 'run' mode. 
		Running is implemented in other versions of Angband by a default macro 
		(part of Angband itself, not the macros accessible through Square) on the 
		Shift + numpad keys.  For this PSP version though, it directly invokes the 
		run command, '.'.  If one were to map a different (Angband core) macro to 
		those keys, the game would ignore the change and continue to run on Right 
		Trigger + direction.

	Select - Cycle targets
		Maps to the asterisk key. Press repeatedly to cycle through nearby targets.

	Start - Lock on target
		Maps to the 't' key. After using Select to choose a target, press Start 
		and you will lock on to that target.  Then in the future, when you fire an 
		arrow	or cast a spell, you can target that enemy by again pressing Start



---Known Issues---
	Many features have not been tested (wizard / borg, characters besides mages, 
		etc...).  They are quite possibly buggy, and in some cases, such as wizard
		mode, I am fairly certain they will crash the game.  Most of the normal
		stuff should work (I have a level 21 character and haven't had any 
		problems), but please be aware that it's entirely possible a bug could wipe
		your save or other files, or even destroy your PSP.  

	Suspend by the power button should now be working, though saving before hand
		is a good idea. 



---Bug reports--
	I have created a thread on the dcemu forums for bug reports.  Feel free to 
		talk to me here: http://www.dcemu.co.uk/vbulletin/showthread.php?t=7856



---Acknowledgements---
	Many thanks to the folks at forums.ps2dev.org - they're always willing to 
		help.  Also thanks to the psptoolchain / pspsdk developers and contributors 
		as their work greatly simplified this project.  Finally, thanks to 
		Mirakichi	for putting out Rogue Clone II.  I'd been toying with the idea 
		of porting Angband for a couple days when RCII came out but playing his 
		port convinced me to go through with mine.



---History---
	(01/10/2010)
		- Updated to Angband 3.0.95 [Daddy32]
	v0.4 (07/21/2005)
		- Loading and saving should be much faster now, as well as responsiveness in 
				the help system [Gendal]
		- Removed overclock during loading, no longer necessary with general speed
				improvements [Gendal]
		- Removed all fixed path's in code. Should now be possible to rename directory 
				to whatever you please. Note older versions are unable to operate in
				anything other than an "Angband" directory [Gendal]
		- PSP bmp screen dump will now ask for a filename. Loading screendumps will not
				work however. [Gendal]
		- If an error causes Angband to call it's quit function, pause and let the user 
				see the error, as well as exit more gracefully. [Gendal]
		- Fixed local mktime implementation which would sometimes result in regenerating 
				the template files when it wasn't really necessary. [Gendal]
		- 
	v0.3 (07/17/2005)
		- Fixed the slow loading.  Now loads in just a second or two unless you
				delete certain files and it has to recreate them. [Gendal]
		- Expanded the view to be 80x33 instead of 80x25.  Note: If you continue
				a character from v0.1 or 0.2, your town will still be the old size.
				The next time you start a character the town will be larger [Gendal]
		- The bottom line of the display is now reserved for the battery meter 
				and	the revamped macro interface. [abszero]
		- The macro 'stars' have been flattened to fit into a single line.  They
				still behave the same way, just the five entries are all in a row. 
				It's a little less intuitive at first, but the macro commands are
				intended to be short enough to memorize anyway. [abszero]
		- For the time being, SELECT is mapped to '*' (cycle through targets) and
				START is mapped to 't' (choose target) [abszero]
		- Font changes (new PSP buttons) and additions (arrow keys) [abszero]
		- Code cleanup and commenting [abszero]

	v0.2 (07/16/2005)
		- Fixed bug that made it difficult/impossible to change the top level 
				macros.
		- Fixed a display bug in the macro editing instructions.
		- Fixed a bug where editing a macro automatically executed the macro too.
		- Added a battery indicator.  Meter in the bottom right corner indicates
				remaining power and says whether or not the battery is charging (AC).  
				As the battery drains, the indicator shifts from blue to red to help 
				prevent the battery going out	by surprise.
		- Now runs at 333 MHz during the slow startup phase, cutting the load time
				from a little over a minute to 40 seconds.  Once it reaches the point
				where it either loads/creates a character, it goes back to 222 MHz.
		- Fixed up the thread priorities and such so that the Home button works
				reliably now.  Suspending still crashes the game though.
		- Makefile now has a 'release' target that compiles the release folders
		- Screenshot function now available.  Unlike the html screenshot most 
				versions of Angband take, this one records the actual image data on the
				screen (including the OSK, etc if they're visible), and saves it to a
				bitmap in the lib/user directory.  The screenshot command is '(' as is
				normal for Angband.

	v0.1 (07/15/2005)
		-Initial release



---Compilation Notes---
	v0.4 (07/21/2005) PSPSDK updated to a version off svn 7/20/2005.
	
	To compile Angband for PSP, I used the toolchain and PSPSDK from ps2dev.org.
		I used a version pulled from the official svn repository early on 
		07/11/2005.  The PSPSDK is under constant revision, and because I do not 
		use the build.mak file they provide, Angband for PSP may not compile on 
		later versions unmodified.  If you run into trouble compiling your own 
		version, check that the LIBS in Makefile.psp include all the libs that 
		build.mak includes, as this is the most likely source of problems.  

	The build environment was Cygwin 1.5.18-1 running under Windows XP SP2

	In anticipation of submitting this port back to thangorodrim.net, the 
		Makefile is named Makefile.psp.

	Currently Makefile.psp does not compile the 'tolua' program that generates
		the l-xxxxxx.c files.  These c files are included with the other source 
		files however, and were included in the original Angband 3.0.6 source 
		tarball downloaded from http://www.thangorodrim.net .  If you find a need
		to rebuild them, I believe you can use a Makefile for a different system 
		(say Makefile.lsl for linux) and it should generate those files.  

	

---Legal---
PSP port:
	Angband was ported to PSP by Mark Dykstra <mdykstra21@hotmail.com>.  You may
	do as you like with the source code, subject to the licensing restrictions 
	below.  By putting this program on a PSP or running it, you accept all 
	consequences including the loss of files on your PSP and computer, 'bricking'
	of your PSP, or worse.


PSPSDK:
This program uses the pspsdk which has the following copyright
restrictions

Copyright (c) 2005  adresd
Copyright (c) 2005  Marcus R. Brown
Copyright (c) 2005  James Forshaw
Copyright (c) 2005  John Kelley
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The names of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


Original Angband source:
This program is based on the Angband source code, which is

Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke

This software may be copied and distributed for educational, research,
and not for profit purposes provided that this copyright and statement
are included in all such copies.  Other copyrights may also apply.


All changes made by Ben Harrison, Robert Ruehlmann, and many other Angband
developers are also available under the GNU GENERAL PUBLIC LICENSE.
Note that this doesn't influence the current distribution, since parts of
the source are still only available under the old Moria/Angband license.
Until all parts of Angband are distributed under the GPL the only valid
license remains the original Moria/Angband license.

More informations about Angband and the GPL can be found at:
http://www.thangorodrim.net/development/opensource.html


