#!/bin/sh

g++ debug.cc -fPIC -shared -o libdebug.so
gcc test.c -L. -ldebug
