set VERSION=v0.6.1

rmdir mc.usb-win-%VERSION%

mkdir mc.usb-win-%VERSION%
mkdir mc.usb-win-%VERSION%\MakeController-externals
mkdir mc.usb-win-%VERSION%\MakeController-help
mkdir mc.usb-win-%VERSION%\max4
xcopy max4 mc.usb-win-%VERSION%\max4 /E
copy MakeController-externals mc.usb-win-%VERSION%\MakeController-externals
delete mc.usb-win-%VERSION%\MakeController-externals\*.mxo
copy msvc\Release\mc.usb.mxe mc.usb-win-%VERSION%\MakeController-externals
copy mc.usb-win-%VERSION%\MakeController-externals\*.mxe mc.usb-win-%VERSION%\max4\MakeController-externals
copy MakeController-help mc.usb-win-%VERSION%\MakeController-help
copy mc-objectlist.txt mc.usb-win-%VERSION%
copy ReadMe-WIN.rtf mc.usb-win-%VERSION%\ReadMe.rtf
