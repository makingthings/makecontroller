#!/bin/sh

# script for generating an mcbuilder release on OS X
# - the app itself is universal but the toolchains are not, so we need 2 separate distrbutions
# - include all the firmware source (export from last stable tag in svn)

REVISION=`head -1 ../../../version.txt`

echo Creating mcbuilder v$REVISION OS X distribution...

# remove any previous copies
rm -Rf mcbuilder
rm -Rf *.dmg

#prepare the app
cp Info.plist ../../../bin/mcbuilder.app/Contents/Info.plist
sed -e s/@@version@@/$REVISION/g -i '' ../../../bin/mcbuilder.app/Contents/Info.plist
strip ../../../bin/mcbuilder.app/Contents/MacOS/mcbuilder

# put it in the right spot and create the appropriate directories
mkdir mcbuilder
mkdir mcbuilder/resources
cp -r ../../../bin/mcbuilder.app mcbuilder
cp -r ../../examples mcbuilder/resources
cp -r ../../board_profiles mcbuilder/resources
cp -r ../../templates mcbuilder/resources

echo getting firmware source...
mkdir mcbuilder/resources/cores
mkdir mcbuilder/resources/cores/makecontroller
# ...do an svn export of the latest tag in the MakingThings firmware SVN repo first
cp -r firmware/controller/* mcbuilder/resources/cores/makecontroller

echo getting reference material...
mkdir mcbuilder/resources/reference
cp -r firmware/doc mcbuilder/resources/reference/makecontroller
cp ../../reference/manual.pdf mcbuilder/resources/reference/manual.pdf

# add the tools - do Intel first, we'll do PPC afterwards
mkdir mcbuilder/resources/tools
echo unzipping intel tools...
unzip -q ../../tools/osx-intel.zip -d tmp
mv tmp/osx-intel/* mcbuilder/resources/tools
rm -Rf tmp

# add the core libraries
mkdir mcbuilder/libraries

# remove any crap
find mcbuilder -name "*~" -exec rm -f {} ';'
find mcbuilder -name "._*" -exec rm -f {} ';'
find mcbuilder -name "Thumbs.db" -exec rm -f {} ';'
find mcbuilder -name ".svn" -exec rm -rf {} ';' 2> /dev/null

# create the .dmg
mkdir mcbuilder-v$REVISION
mv mcbuilder mcbuilder-v$REVISION/mcbuilder
ln -s /Applications mcbuilder-v$REVISION # add a link to the Applications directory
cp ../../../ReadMe.rtf mcbuilder-v$REVISION

echo creating intel .dmg...
#hdiutil create -fs HFS+ -srcfolder "./mcbuilder-v$REVISION/" -volname "mcbuilder-v$REVISION" "mcbuilder-intel-v$REVISION.dmg"

#rm -Rf mcbuilder-v$REVISION/mcbuilder/resources/tools/*
#cp -r ../../tools/osx-intel mcbuilder-v$REVISION/mcbuilder/resources/tools
echo creating ppc .dmg...
#hdiutil create -fs HFS+ -srcfolder "./mcbuilder-v$REVISION/" -volname "mcbuilder-v$REVISION" "mcbuilder-ppc-v$REVISION.dmg"

#rm -Rf mcbuilder-v$REVISION

echo Done.
