Runes
=====

As a programmer, I spend the vast majority of my time on a computer in a
terminal window. This has always meant dealing with a wide variety of
limitations related to the fact that a "terminal emulator" really is exactly
that - at its core, it's emulating a decades-old hardware terminal interface.
There's no reason why at this point we need to be tied to those old APIs, but
all of the attempts I've seen so far to move away from that have involved
rewriting the whole thing from the ground up, breaking compatibility with all
of the existing terminal applications out there.

This is why I decided to write Runes. Runes is a new terminal emulator with a
goal not to fully emulate some ancient piece of hardware, but to support
enough existing terminal control codes to run modern terminal software, and
also to become a place to experiment with new features for terminal
applications. There's no reason why terminals can't use actual graphics, or
support mouse-based interfaces (better than xterm, which only supports
clicks), or many other things like that which currently require switching to
an entirely different application to run.

Some amount of portability is also a goal. Rather than being Linux-specific,
I'd like to be able to support at least recent versions of Linux, OSX,
FreeBSD, and (maybe) Windows. Currently, only Linux is supported.

Right now it is in a very early stage (I'm still in the process of
implementing enough existing terminal functionality to be useful), but it
works well enough for basic interactions (nethack is playable, for instance).
Once enough functionality has been implemented to make it usable, I'll start
looking into new extensions.

Building
--------
Runes requires cairo, xlib, and libuv. Once those dependencies are installed,
you should be able to build it just by running `make`. If you're interested in
modifying the parser, you'll also need to install flex.

Contributing
------------
You can report bugs and submit pull requests to https://github.com/doy/runes/.
You can also contact me with questions, ideas, or patches at @doyster on
twitter, or doy@tozt.net via email.
