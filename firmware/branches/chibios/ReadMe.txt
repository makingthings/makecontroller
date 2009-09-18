Make Controller Kit - Firmware

For detailed information about how to get started writing programs for the Make Controller, check the Programming the Make Controller tutorial on the MakingThings site.
http://www.makingthings.com/documentation/tutorial/programming-the-make-controller-kit

Contents
*********
The Make Controller firmware codebase provides a simple software interface to writing programs for the Make Controller.  The core directory contains code that provides the basic functionality of the Make Controller, and the libraries directory contains a variety of modules that provide additional functionality that can be added/removed easily depending on a project's needs.

The projects directory contains a few sample projects:

  Heavy - the default full-featured firmware shipped with the Make Controller Kit.  It's based on FreeRTOS and includes networking, USB, OSC, ADC, PWM, CAN, RS232, SPI, motor control, EEPROM, and high resolution timing.

  Tiny - a demonstration of firmware that doesn't make use of FreeRTOS.  It's meant as a point of departure for those developing their own firmware that does not use a real-time operating system.  

  Solo - a demonstration of firmware that uses only the controller library.  It's meant as a point of departure for those developing their own versions of the Application Board that the Controller Board can plug into.  As you might guess, since the Application Board is assumed to not be present, none of the Application Board utilities are available.

The entire firmware directory can be kept wherever is most convenient, and you can easily create your own projects by saving a copy of one of the existing ones and starting to modify it.  Your projects should be located in the projects directory unless you adjust your Makefile accordingly.

Building
**********
Each project directory includes an output directory, where the final image is created.  To build any of the projects, you'll need to have an appropriate toolchain installed - download one at www.makingthings.com/resources/downloads.  GnuArm.org also has a variety of packages that tend to be kept reasonably up to date.

With the toolchain installed, navigate from the command line to the directory of the project you'd like to build and type make.  

cd /path/to/firmware/project
make

If all goes well, you'll have a shiny new .bin file in your output directory ready to be uploaded to the Make Controller.  Use mchelper to actually upload the firmware to the board - also available at MakingThings.com.  Check the forums with any questions or problems.  

The .hzp files are project files for Rowley's Crossworks for ARM.  If you happen to use this IDE, then you can simply open up the project and build it.  One of the most significant benefits to using an environment like this is the capability to do single-step debugging on your projects.  Otherwise, the GCC-based approach produces very similar results.

