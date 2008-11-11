#!/bin/sh

# first build the test
cd ..
qmake "CONFIG += test_suite" mchelper.pro
make
echo # some space

# then run it...
tests/mchelper_test.app/Contents/MacOS/mchelper_test

echo # more space

