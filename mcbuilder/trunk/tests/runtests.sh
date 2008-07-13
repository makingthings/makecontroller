#!/bin/sh

# first build the test
qmake
make
echo # some space

# then run it...
TestMcbuilder.app/Contents/MacOS/TestMcbuilder

echo # more space

