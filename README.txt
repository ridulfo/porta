
██████   ██████  ██████  ████████  █████  
██   ██ ██    ██ ██   ██    ██    ██   ██ 
██████  ██    ██ ██████     ██    ███████ 
██      ██    ██ ██   ██    ██    ██   ██ 
██       ██████  ██   ██    ██    ██   ██ 
                                          
Dead-simple, ultra-portable word processor
The intended use case is just writing. There are no efficient Vim-like bindings
on purpose. This is not for editing; that can happen somewhere else.

I made this mainly to learn "old C" and to exploring how portable a terminal
word processor can be.

Scope:
------
- C99
- POSIX.1-2001
- VT100 terminal control

If $TERM is "xterm-kitty" then it supports text sizing.
See https://sw.kovidgoyal.net/kitty/text-sizing-protocol

Features:
---------
- censored mode for writing in public
- centered text
- one file at a time

Build Instructions:
-------------------
git clone https://github.com/ridulfo/porta.git
cd porta
make

To install:
make install

This will produce an executable in the build directory.

License:
--------
GPLv3 License.


