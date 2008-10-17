mc.usb v0.5 - src
USB connection to the Make Controller Kit from Max/MSP

mc.usb connects to the Make Controller Kit via USB and can send/receive OSC messages.

Contents
This package contains the source files to create the mc.usb external on OS X and Windows.

Building
mc.usb is built with Xcode on OS X, and with Visual Studio on Windows - project files for each are included.  It's maintained for both OS X and Windows via #ifdef conditionalization for platform specific segments of code.

On OS X, the Max SDK expects you to maintain a particular directory structure with your projects - in the main MaxSDK directory, there are all the sample projects directories, the WritingExternals,pdf file and a few other goodies.  I've set my machine up to fit in with this - so, I would check out the mc.usb source to a directory that's located at MaxSDK/mc-projects/mc.usb.  You're of course welcome to set your system up however you like, but a the Xcode project will expect to live in this kind of structure, so make any changes if appropriate.

On Windows, I've used MS Visual C++ to compile the code.  Again, you'll need to maintain a directory structure as described above in order for the appropriate files to be found.  

Lastly, on Windows you need to install USB drivers - this is done with the mchelper installer.  Head to www.makingthings.com/resources/downloads and grab the latest version of mchelper, and run the installer.  It's best to do this before plugging in your Make Controller for the first time - otherwise, Windows likes to create a stub of a USB device in the device manager that then usually needs to be deleted.  OS X does not require a driver installation.

Support
The best place to ask questions is the forum on the MakingThings site - http://www.makingthings.com/forum.  There is also an IRC channel #makingthings on irc.freenode.net where Make Controller enthusiasts are known to congregate.  The MakingThings tracker can be found at http://dev.makingthings.com and has outstanding issues, requests, roadmap and code browser.