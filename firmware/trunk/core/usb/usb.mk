
# USB framework

USBCORESRC =    $(USB)/usb/device/core/USBDCallbacks_Initialized.c \
                $(USB)/usb/device/core/USBDDriverCb_CfgChanged.c \
                $(USB)/usb/device/core/USBDDriverCb_IfSettingChanged.c \
                $(USB)/usb/device/core/USBDDriver.c \
                $(USB)/usb/device/core/USBD_UDP.c \
                $(USB)/usb/common/core/USBSetAddressRequest.c \
                $(USB)/usb/common/core/USBGenericDescriptor.c \
                $(USB)/usb/common/core/USBGenericRequest.c \
                $(USB)/usb/common/core/USBGetDescriptorRequest.c \
                $(USB)/usb/common/core/USBSetConfigurationRequest.c \
                $(USB)/usb/common/core/USBFeatureRequest.c \
                $(USB)/usb/common/core/USBEndpointDescriptor.c \
                $(USB)/usb/common/core/USBConfigurationDescriptor.c \
                $(USB)/usb/common/core/USBInterfaceRequest.c

USBCDCSRC =     $(USB)/usb/device/cdc-serial/CDCDSerialDriver.c \
                $(USB)/usb/device/cdc-serial/CDCDSerialDriverDescriptors.c \
                $(USB)/usb/common/cdc/CDCSetControlLineStateRequest.c \
                $(USB)/usb/common/cdc/CDCLineCoding.c

USBHIDCORE =    $(USB)/usb/common/hid/HIDIdleRequest.c \
                $(USB)/usb/common/hid/HIDReportRequest.c

USBHIDKB =      $(USB)/usb/device/hid-keyboard/HIDDKeyboardDriver.c \
                $(USB)/usb/device/hid-keyboard/HIDDKeyboardDriverDescriptors.c \
                $(USB)/usb/device/hid-keyboard/HIDDKeyboardInputReport.c \
                $(USB)/usb/device/hid-keyboard/HIDDKeyboardOutputReport.c \
                $(USB)/usb/device/hid-keyboard/HIDDKeyboardCallbacks_LedsChanged.c

USBHIDMOUSE =   $(USB)/usb/device/hid-mouse/HIDDMouseDriver.c \
                $(USB)/usb/device/hid-mouse/HIDDMouseDriverDescriptors.c \
                $(USB)/usb/device/hid-mouse/HIDDMouseInputReport.c

# include directories

USBINC =    		$(USB)

