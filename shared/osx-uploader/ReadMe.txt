On OS X 10.5 and above, the OS X USB system has introduced a change that disables uploading new firmware to the Make Controller.  This installer includes a fix for this.

Remember - this is a one time installation.  All the different programs that upload firmware to your Make Controller (mchelper, mcbuilder, sam7) can all take advantage of the change applied by this installer, so you only need to run it once.  Running it more than once is no problem - just not necessary.

To uninstall the patch, open up /Applications/Utilities/Terminal.app and type

sudo rm -rf /System/Library/Extensions/sam7utils.kext

and when you next restart your machine, the patch will be unloaded.
