set VERSION=v0.5

rmdir mc.usb-win-%VERSION%

mkdir mc.usb-win-%VERSION%
mkdir mc.usb-win-%VERSION%\MakeController-externals
mkdir mc.usb-win-%VERSION%\MakeController-help
copy MakeController-externals mc.usb-win-%VERSION%\MakeController-externals
copy msvc\Release\mc.usb.mxe mc.usb-win-%VERSION%\MakeController-externals
copy MakeController-help mc.usb-win-%VERSION%\MakeController-help
copy mc-objectlist.txt mc.usb-win-%VERSION%
copy ReadMe-WIN.rtf mc.usb-win-%VERSION%\ReadMe.rtf
