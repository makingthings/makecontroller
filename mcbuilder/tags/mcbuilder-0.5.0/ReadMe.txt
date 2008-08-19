mcbuilder 0.2.0

* Notes
mcbuilder is written with Qt - you'll need at least Qt 4.4.0 installed in order to build it.  Other than that, there are no special dependencies beyond gcc or another suitable C++ compiler.  Once you have Qt installed and added to your path, cd into the root mcbuilder directory and type 'qmake' and then 'make' in order to build it.

Depending on your setup, you may need to move/copy some files from the 'resources' directory around in order for mcbuilder to operate properly.  Specifically, the 'libraries' directory, and the 'resources' dir, with 'examples', 'board_profiles', 'reference', 'templates', and 'tools'  it it, should be located at the same directory level that mcbuilder itself is.  

You'll also need to grab the latest stable firmware source from the MakingThings SVN repo , which for the moment is at https://makingthings.svn.sourceforge.net/svnroot/makingthings/firmware/tags/firmware-v1.3.1.  A bit of reorganization is required at the moment as the firmware source tree is undergoing a few changes, but you can download the os x or windows binaries to see how the source tree should be organized.  This will be fixed soon so that the firmware source tree can be dropped directly into mcbuilder.

Please post any questions to the MakingThings forum - http://www.makingthings.com/forum