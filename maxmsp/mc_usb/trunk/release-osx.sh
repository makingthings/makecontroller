#!/bin/sh

# script for generating an mc.usb release on OS X

#grab the version
VERSION=v0.6.1

echo Creating mcbuilder $VERSION OS X distribution...

# remove any previous copies
rm -Rf mc.usb-$VERSION
rm -Rf *.dmg
rm -Rf *.zip

# put it in the right spot and create the appropriate directories
mkdir mc.usb-$VERSION
cp ReadMe-OSX.rtf mc.usb-$VERSION/ReadMe.rtf
mkdir mc.usb-$VERSION/Max4  # need to send along versions of abstractions and help patches for Max 4 as well.
cp -r max4 mc.usb-$VERSION
cp -r MakeController-externals mc.usb-$VERSION
cp -r build/Deployment/mc.usb.mxo mc.usb-$VERSION/MakeController-externals
cp -r mc.usb-$VERSION/MakeController-externals/*.mxo mc.usb-$VERSION/max4/MakeController-externals
cp -r MakeController-help mc.usb-$VERSION
rm mc.usb-$VERSION/MakeController-externals/*.mxe
cp mc-objectlist.txt mc.usb-$VERSION

# remove any crap
find mc.usb-$VERSION -name "*~" -exec rm -f {} ';'
find mc.usb-$VERSION -name "._*" -exec rm -f {} ';'
find mc.usb-$VERSION -name "Thumbs.db" -exec rm -f {} ';'
find mc.usb-$VERSION -name ".svn" -exec rm -rf {} ';' 2> /dev/null

# create the .dmg
echo creating .dmg...
hdiutil create -fs HFS+ -srcfolder "./mc.usb-$VERSION/" -volname "mc.usb-$VERSION" "mc.usb-$VERSION.dmg"

# create the source release
echo Creating source package
mkdir mc.usb-src
cp -r src mc.usb-src
cp -R msvc mc.usb-src
cp -R mc.usb.xcodeproj mc.usb-src
rm mc.usb-src/mc.usb.xcodeproj/*.pbxuser
rm mc.usb-src/mc.usb.xcodeproj/*.mode*
cp ReadMe-dev.txt mc.usb-src/ReadMe.txt
# remove any crap
find mc.usb-src -name "*~" -exec rm -f {} ';'
find mc.usb-src -name "._*" -exec rm -f {} ';'
find mc.usb-src -name "Thumbs.db" -exec rm -f {} ';'
find mc.usb-src -name ".svn" -exec rm -rf {} ';' 2> /dev/null

zip -rq mc.usb-src-$VERSION.zip mc.usb-src 

rm -Rf mc.usb-$VERSION
rm -Rf mc.usb-src

echo Done.
