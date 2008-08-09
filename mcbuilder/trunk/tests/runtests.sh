#!/bin/sh

# first build the test
cd ..
qmake "CONFIG += test_suite" mcbuilder.pro
make
echo # some space

# then run it...
tests/mcbuilder_test.app/Contents/MacOS/mcbuilder_test

echo # more space

