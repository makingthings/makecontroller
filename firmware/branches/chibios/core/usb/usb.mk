
# List of all the at91lib USB files needed (only usb serial for the moment)

USBSRC = $(USB)/device/cdc-serial/CDCDSerialDriver.c \
          $(USB)/device/cdc-serial/CDCDSerialDriverDescriptors.c \
          $(USB)/device/core/USBDCallbacks_Initialized.c \
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
          $(USB)/common/core/USBInterfaceRequest.c \
          $(USB)/common/cdc/CDCSetControlLineStateRequest.c \
          $(USB)/common/cdc/CDCLineCoding.c \

# Required include directories
USBINC = $(USB)/device/cdc-serial \
          $(USB)/device/core \
          $(USB)/common/core \
          $(USB)/common/cdc