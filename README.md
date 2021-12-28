This project acts as a container for a bunch of useful utility functions
for C programs.

It doesn't do anything by itself, but it's quite handy when writing
other software.

It was written by Idan Shoham and other open source code from Idan tends
to depend on it - i.e., to build the other things, you need to install
this first.

There are utility functions for filesystem operations, launching and
interacting with child processes, for simple cryptography, for string
and date manipulation, etc.  All pretty common and useful stuff.

There is a built-in set of error handling functions which are used heavily
elsewhere.

There is also a built-in set of functions for nested linked lists (tag/value
structures).  There is a hand-crafted JSON parser and generator that uses
this as the input/output format.

Author: Idan Shoham - 00shoham (you know what) gmail.com.  (c) 2021
