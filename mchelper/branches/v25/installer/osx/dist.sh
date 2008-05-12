#!/bin/sh

REVISION=`head -1 ../../version.txt`

echo Creating mchelper v$REVISION OS X distribution...

# first do the OS X distribution

# remove any previous copies
rm -Rf mchelper
rm -Rf *.dmg

#prepare the app itself
sed -e s/@@version@@/$REVISION/g -i '' ../../bin/mchelper.app/Contents/Info.plist
strip ../../bin/mchelper.app/Contents/MacOS/mchelper

# put it in the right spot
mkdir mchelper
cp -r ../../bin/mchelper.app mchelper
cp ../../ReadMe.rtf mchelper

# add a link to the Applications directory
ln -s /Applications mchelper

# remove any crap
find mchelper -name "*~" -exec rm -f {} ';'
find mchelper -name "._*" -exec rm -f {} ';'
find mchelper -name "Thumbs.db" -exec rm -f {} ';'
find mchelper -name ".svn" -exec rm -rf {} ';' 2> /dev/null

mv mchelper mchelper-v$REVISION
hdiutil create -fs HFS+ -srcfolder "./mchelper-v$REVISION/" -volname "mchelper-v$REVISION" "mchelper-v$REVISION.dmg"

rm -Rf mchelper-v$REVISION

echo Done.
