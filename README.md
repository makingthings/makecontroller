# makecontroller
firmware and tools for the Make Controller Kit

The Make Controller is an open source network-enabled microcontroller platform, based on the Atmel 32-bit SAM7X, sponsored by MakingThings. It provides easy to use development tools, a rich set of libraries to get you started quickly and a variety of integrations to desktop environments. The main support site, with forums, documentation and store can be found at http://www.makingthings.com.

There are a variety of software components to the Make Controller project:

## Firmware

The firmware for the Make Controller is written in C/C++ and provides a variety of libraries for easily controlling the board's peripherals, communicating on the network, via USB and much more. Docs are available at http://www.makingthings.com/ref/firmware/html.

We're currently exploring a transition of the firmware to use the ChibiOS operating system: see the chibios branch for details, and help out if you like!

## mchelper

Make Controller Helper is a configuration program that makes it easy to communicate with the default firmware build for the Make Controller, called Heavy. Heavy communicates with mchelper via OSC (Open Sound Control) and exposes most of the functionality on the board to external control.

mchelper also provides an XML server that can manage communication between the Make Controller and other desktop applications if those applications can't or don't want to communicate directly via USB or the network interface.

mchelper is written in C++/Qt4.

## mcbuilder

Make Controller Builder is an easy to use, all in one development environment for the Make Controller. It provides a pre-configured toolchain for cross compilation, simple code editor and project management for Make Controller firmware development.

mcbuilder is written in C++/Qt4.

## Flash / ActionScript

The Make Controller can communicate with Flash & ActionScript projects via the XMLSocket API. This requires running mchelper since Flash can't communicate directly via USB, for example. Docs are available at http://www.makingthings.com/ref/flash.

## Max/MSP

The Make Controller can communicate via USB with Max/MSP using the mc.usb external. If you don't need to communicate via USB, you can use the built-in udpsend and udpreceive objects along with the OpenSoundControl object from CNMAT.
