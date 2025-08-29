
██████   ██████  ██████  ████████  █████  
██   ██ ██    ██ ██   ██    ██    ██   ██ 
██████  ██    ██ ██████     ██    ███████ 
██      ██    ██ ██   ██    ██    ██   ██ 
██       ██████  ██   ██    ██    ██   ██ 
                                          
Dead-simple, ultra-portable word processor
The intended use case is just writing. No efficient Vim-like bindings on
purpose. This is not for editing, only for getting words out of your head.

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
- censored mode for writing in public places
- one file at a time
- centered text
- append only

Build Instructions:
-------------------
git clone https://github.com/ridulfo/porta.git
cd porta
make

To install:
make install


License:
--------
GPLv3 License.


