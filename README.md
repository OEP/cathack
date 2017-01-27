cathack
=======

`cathack` is a Unix tool for making you type like they do in the movies. Like
`cat`, it echoes the files you pass to standard out, but incrementally as you
mash keys on the keyboard as fast as humanly possible.

Building
--------

Nothing fancy:

    make

Examples
--------

General usage

    cathack [options] file [file...]

Vanilla mode

    cathack cathack.c

Loop forever

    cathack -l cathack.c

Clear the screen first

    cathack -c cathack.c

Change how many characters are echoed per keystroke

    cathack -f 6 cathack.c
