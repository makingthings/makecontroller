#!/bin/sh

# first build the test
cd ..
qmake -spec macx-g++ "CONFIG += test_suite" mcbuilder.pro
make -j2
if [ "$?" -ne "0" ]; then
  # don't run the tests if the build didn't even succeed
  exit 1
fi
echo # some space

# then run it...
tests/mcbuilder_test.app/Contents/MacOS/mcbuilder_test

echo # more space

