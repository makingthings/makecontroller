#!/bin/sh

# install the kext in /System/Library/Extensions & load it up
sudo /usr/sbin/chown -R root:wheel /System/Library/Extensions/sam7utils.kext
sudo find /System/Library/Extensions/sam7utils.kext -type d -exec /bin/chmod 0755 {} \;
sudo find /System/Library/Extensions/sam7utils.kext -type f -exec /bin/chmod 0644 {} \;
sudo kextload -v /System/Library/Extensions/sam7utils.kext

# that's it!

