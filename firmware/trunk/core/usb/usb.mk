
# USB framework

USBCORESRC =    $(USB)/device/core/USBDCallbacks_Initialized.c \
                $(USB)/device/core/USBDCallbacks_Reset.c \
                $(USB)/device/core/USBDCallbacks_Resumed.c \
                $(USB)/device/core/USBDCallbacks_Suspended.c \
                $(USB)/device/core/USBDDriverCb_CfgChanged.c \
                $(USB)/device/core/USBDDriverCb_IfSettingChanged.c \
                $(USB)/device/core/USBDDriver.c \
                $(USB)/device/core/USBD_UDP.c \
                $(USB)/common/core/USBSetAddressRequest.c \
                $(USB)/common/core/USBGenericDescriptor.c \
                $(USB)/common/core/USBGenericRequest.c \
                $(USB)/common/core/USBGetDescriptorRequest.c \
                $(USB)/common/core/USBSetConfigurationRequest.c \
                $(USB)/common/core/USBFeatureRequest.c \
                $(USB)/common/core/USBEndpointDescriptor.c \
                $(USB)/common/core/USBConfigurationDescriptor.c \
                $(USB)/common/core/USBInterfaceRequest.c

USBCDCSRC =     $(USB)/device/cdc-serial/CDCDSerialDriver.c \
                $(USB)/device/cdc-serial/CDCDSerialDriverDescriptors.c \
                $(USB)/common/cdc/CDCSetControlLineStateRequest.c \
                $(USB)/common/cdc/CDCLineCoding.c

USBHIDCORE =    $(USB)/common/hid/HIDIdleRequest.c \
                $(USB)/common/hid/HIDReportRequest.c

USBHIDKB =      $(USB)/device/hid-keyboard/HIDDKeyboardDriver.c \
                $(USB)/device/hid-keyboard/HIDDKeyboardDriverDescriptors.c \
                $(USB)/device/hid-keyboard/HIDDKeyboardInputReport.c \
                $(USB)/device/hid-keyboard/HIDDKeyboardOutputReport.c \
                $(USB)/device/hid-keyboard/HIDDKeyboardCallbacks_LedsChanged.c

USBHIDMOUSE =   $(USB)/device/hid-mouse/HIDDMouseDriver.c \
                $(USB)/device/hid-mouse/HIDDMouseDriverDescriptors.c \
                $(USB)/device/hid-mouse/HIDDMouseInputReport.c

# include directories

USBCOREINC =    $(USB)/device/core \
                $(USB)/common/core  

USBCDCINC =     $(USB)/device/cdc-serial \
                $(USB)/common/cdc

USBHIDCOREINC = $(USB)/common/hid

USBHIDKBINC =   $(USB)/device/hid-keyboard

USBHIDMOUSEINC = $(USB)/device/hid-mouse

