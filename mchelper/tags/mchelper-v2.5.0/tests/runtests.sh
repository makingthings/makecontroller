#!/bin/sh

# first build the test
cd ..
qmake -spec macx-g++ "CONFIG += test_suite" mchelper.pro
make
if [ "$?" -ne "0" ]; then
  # don't run the tests if the build didn't even succeed
  exit 1
fi
echo # some space

# then run it...
tests/mchelper_test.app/Contents/MacOS/mchelper_test

echo # more space

