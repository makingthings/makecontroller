#!/bin/sh

# first build the test
# qmake -project "CONFIG += console qtestlib"
qmake
make

# then run it...
open ./tests.app/Contents/MacOS/tests # OS X




