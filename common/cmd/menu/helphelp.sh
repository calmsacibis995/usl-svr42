#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)menu.cmd:helphelp.sh	1.1"
#ident	"$Header: $"
This help screen will provide you with information on the following topics:

    SPECIAL KEYS
    HELP SCREENS
    THE INSTALLATION PROCESS

SPECIAL KEYS

Several keys have special meanings while installing the UNIX System.

The following keys can be used while in the help facility:

    '1' - display the next page of text.  This is signified by the
    "1=Forward" displayed in the help bar near the bottom of the screen.

    '2' - display the previous page of text.  This is signified by the
    "2=Back" displayed in the help bar near the bottom of the screen.

    'ESC' - return from help to the installation program (or from this
    screen to the original help screen).  This is signified by the
    "ESC=Exit help" (or the "ESC=Exit Instructions") displayed in the help
    bar near the bottom of the screen.

The following keys can be used at any time during the installation process:

    'Del' (or 'Delete') - cancel the installation.  This is signified by the
    "Del=Cancel" displayed in the help bar near the bottom of the screen.
    Note that if you cancel the installation, you will have to re-start the
    installation before you can use the UNIX System.  It may not be possible
    to cancel installation at all times, in which case you will see a
    warning message to that effect.

    'F1' (or '?') - Display help.  You may also the use the '?' key.  This
    is signified by the "F1=Help" displayed in the help bar near the bottom
    of the screen.  Once you have pressed 'F1' to view the help screen, this
    will change to "F1=Instructions," indicating that pressing 'F1' a second
    time will present the screen of general instructions that you are seeing
    now.

    The 'TAB' key moves the cursor (the line or block on the screen - which
    may be blinking) to the field to the right of the current field.

    The 'BACK TAB' key (usually 'shift'-'TAB' - hold the 'shift' key down
    while pressing the 'TAB' key) moves the cursor to the field to the left
    of the current field.

    The up arrow key moves the cursor to the field above the current field.

    The down arrow key moves the cursor to the field below the current field.

    The right arrow displays the next choice in a "select field".  If your
    keyboard does not have a right arrow key, the '+' key will display the
    next choice in a "select field".

    The left arrow displays the previous choice in a "select field".  If your
    keyboard does not have a left arrow key, the '-' key will display the
    previous choice in a "select field".

HELP SCREENS

There are two help screens that may be read from each step in the
installation process.  The first contains information specific to the screen
that you are on when you press 'F1'.  If you press 'F1' again while reading
that help screen, you will see this screen of general installation
instructions.

The upper right hand corner of each help screen shows which page of text you
are on (for instance "Page 1 of 3").  When you are on the first page of a
help screen you can not use '2' to go "Back" to a previous page, and when
you are on the last page of a help screen you can not use '1' to go
"Forward" to the next page.

THE INSTALLATION PROCESS

The UNIX System installation process consists of a series of steps during
which you will be asked questions or told what is about to happen.  You
provide the UNIX System with information through "fields".  Fields are
shown as either brighter text or as reverse color text (reverse video for a
monochrome monitor) (depending on the type of field).  Using the keys
described above will move the cursor to each of the fields on the screen.
The field that the cursor is at is the current field and will receive any
input you type on the keyboard.  Brief instructions on how to complete the
current field can be found at the bottom of the screen in the help bar.

There are two types of fields used during the UNIX System installation.

"Select fields" are displayed as brighter text.  Each "select field" has a
list of predefined choices for you to pick from.  Pressing the right arrow
key will display the next choice in the list and pressing the left arrow key
will display the previous choice in the list.  If your keyboard does not have
right and left arrow keys, then the '+' and '-' keys will respectively
display the next and previous choice in the list.  When you get to the end of
the list of choices, the first choice will be displayed.  When you are
satisfied with the choice displayed, press the TAB key to proceed to the
next field.  When a "select field" is the active field, the help bar will
also display the reminder:

    "Right/left arrow keys for new choice (X choices)"

where the X will be replaced with the total number of choices available.

"Typed fields" are displayed in reverse color text (or reverse video text on
a monochrome monitor).  A "typed field" requires that you type in one or more
characters (or words).  When you are satisfied with your selection, press the
TAB key to go to the next field (if any).  If you make a mistake while typing
in your selection, pressing the backspace key will erase one character at a
time (until there are no characters displayed in the "typed field").
Pressing the space bar as the first character in a "typed field" will erase
the contents of the field.  When a "typed field" is the active field, the
help bar will also display a brief description of what to type.

The UNIX System installation also uses "buttons".  "Buttons" are displayed
as a word or words enclosed in a "box".  A "button" is "pushed" (or activated)
by moving the cursor to the "button" (using the 'TAB' or 'BACK TAB' keys)
and then pressing 'ENTER' (or 'RETURN').  Activating the "Apply button"
will cause the installation process to apply your selections and continue
the installation.  Activating the "Reset button" will cause the installation
program to reset all the fields on the form to their original values.

The UNIX System is "case sensitive".  This means that the letters 'A' and 'a'
are not the same to the UNIX System (as they are with some other operating
systems).  It is recommended that you always use lower case letters when
entering selections unless a "typed field" is indicated as accepting upper
and lower case letters.

Most fields will display a default (or "typical") choice.  The default choice
is simply the choice that is most appropriate for the majority of UNIX System
users, but it may not necessarily be the appropriate choice for your
situation.  An example of such a situation might be the default choice for
the type of mouse the UNIX System expects.  The default for the mouse type is
a serial mouse (i.e. a mouse that plugs into one of the serial ports on your
computer).  If you have a Bus mouse (i.e. a mouse that plugs into a type of
controller card that is installed in your computer), using the default
choice of serial mouse would not be appropriate.  To change from the default
choice for a "select field", simply press the left or right arrow keys (or
the '+' or '-' keys if your keyboard does not have arrow keys) until the
desired choice is displayed.  To change the default selection for a "typed
field" either

   - press the space bar to blank out the entire field and then type in a
     new selection, or

   - type your new selection (which will overwrite the default selection
     on the screen).

If you are not sure about whether or not to accept the default selections and
reading both the help information for the current screen and the Destiny
Installation Guide still do not make clear what choice to make, it is
recommended that you accept the default selection.
