#!/bin/sh

# script for generating an mcbuilder release on OS X
# - the app itself is universal but the toolchains are not, so we need 2 separate distrbutions
# - include all the firmware source (export from last stable tag in svn)
# - generate up-to-date documentation (doxygen) 

REVISION=`head -1 ../../../version.txt`

echo Creating mcbuilder v$REVISION OS X distribution...

# first do the OS X distribution

# remove any previous copies
rm -Rf mcbuilder
rm -Rf *.dmg

#prepare the app
cp -f ../../osx/Info.plist ../../../bin/mcbuilder.app/Contents/Info.plist
sed -e s/@@version@@/$REVISION/g -i '' ../../../bin/mcbuilder.app/Contents/Info.plist
strip ../../../bin/mcbuilder.app/Contents/MacOS/mcbuilder

# put it in the right spot and create the appropriate directories
mkdir mcbuilder
cp -r ../../../bin/mcbuilder.app mcbuilder
cp -r ../../examples mcbuilder/examples
mkdir mcbuilder/resources

echo getting firmware source...
mkdir mcbuilder/resources/cores
mkdir mcbuilder/resources/cores/makecontroller
svn export -q https://makingthings.svn.sourceforge.net/svnroot/makingthings/firmware/tags/firmware-v1.3.1/controller mcbuilder/resources/cores/makecontroller/controller
svn export -q https://makingthings.svn.sourceforge.net/svnroot/makingthings/firmware/tags/firmware-v1.3.1/appboard mcbuilder/resources/cores/makecontroller/appboard

echo getting reference material...
mkdir mcbuilder/reference
svn export -q https://makingthings.svn.sourceforge.net/svnroot/makingthings/firmware/tags/firmware-v1.3.1/doc/html mcbuilder/reference/html
cp ../../manual.pdf mcbuilder/reference

# add the tools - do PPC first, we'll do Intel afterwards
mkdir mcbuilder/resources/tools
cp -r ../../board_profiles mcbuilder/resources
cp -r ../../templates mcbuilder/resources

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

echo creating ppc .dmg...
hdiutil create -fs HFS+ -srcfolder "./mcbuilder-v$REVISION/" -volname "mcbuilder-v$REVISION" "mcbuilder-ppc-v$REVISION.dmg"

rm -Rf mcbuilder-v$REVISION/mcbuilder/resources/tools/*
cp -r ../../tools/osx-intel/* mcbuilder-v$REVISION/mcbuilder/resources/tools
echo creating intel .dmg...
hdiutil create -fs HFS+ -srcfolder "./mcbuilder-v$REVISION/" -volname "mcbuilder-v$REVISION" "mcbuilder-intel-v$REVISION.dmg"

rm -Rf mcbuilder-v$REVISION

echo Done.
