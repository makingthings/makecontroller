#
# MAKINGTHINGS MAKE FILE FOR THE MAKE CONTROLLER
# Each project may be named uniquely 
# (See Output variable below) and possibly have different source files
# Dependencies are automatically created by the compiler (into .d files) and 
# included in the makefile 
#

# Name the output files here
OUTPUT = heavy

# this rule says that when "make" is typed the bin file is created 		       
all: output/$(OUTPUT).bin

FREERTOS = ../../core/freertos
LWIP = ../../core/lwip
USB = ../../core/usb
MAKINGTHINGS = ../../core/makingthings
LIBRARIES = ../../libraries

# Source files that can be built in THUMB mode.
THUMB_SRC= \
  ../../core/startup/extras.c \
  $(FREERTOS)/tasks.c \
  $(FREERTOS)/queue.c \
  $(FREERTOS)/list.c \
  $(FREERTOS)/portable/GCC/ARM7_AT91SAM7S/port.c \
  $(FREERTOS)/portable/MemMang/heap_2.c \
  $(MAKINGTHINGS)/main.c \
  $(MAKINGTHINGS)/eeprom.c \
  $(MAKINGTHINGS)/analogin.c \
  $(MAKINGTHINGS)/serial.c \
  $(MAKINGTHINGS)/pwm.c \
  $(MAKINGTHINGS)/spi.c \
  $(MAKINGTHINGS)/USB-CDC.c \
  $(MAKINGTHINGS)/rtos.c \
  $(MAKINGTHINGS)/usb.c \
  $(MAKINGTHINGS)/network.c \
  $(MAKINGTHINGS)/system.c \
  $(MAKINGTHINGS)/io.c \
  $(MAKINGTHINGS)/timer.c \
  $(MAKINGTHINGS)/fasttimer.c \
  $(MAKINGTHINGS)/debugosc.c \
  $(MAKINGTHINGS)/osc.c \
  $(MAKINGTHINGS)/osc_patternmatch.c \
  $(LIBRARIES)/xbee/xbee.c \
  $(LIBRARIES)/servo/servo.c \
  $(LIBRARIES)/stepper/stepper.c \
  $(LIBRARIES)/dipswitch/dipswitch.c \
  $(LIBRARIES)/motor/motor.c \
  $(LIBRARIES)/pwmout/pwmout.c \
  $(LIBRARIES)/digitalin/digitalin.c \
  $(LIBRARIES)/digitalout/digitalout.c \
  $(LIBRARIES)/webserver/webserver.c \
  $(LWIP)/src/core/tcp_out.c \
  $(LWIP)/src/core/ipv4/inet.c \
  $(LWIP)/src/core/mem.c \
  $(LWIP)/src/core/memp.c \
  $(LWIP)/src/core/netif.c \
  $(LWIP)/src/core/pbuf.c \
  $(LWIP)/src/core/raw.c \
  $(LWIP)/src/core/stats.c \
  $(LWIP)/src/core/sys.c \
  $(LWIP)/src/core/tcp.c \
  $(LWIP)/src/core/tcp_in.c \
  $(LWIP)/src/core/init.c \
  $(LWIP)/src/core/ipv4/ip.c \
  $(LWIP)/src/core/ipv4/ip_addr.c \
  $(LWIP)/src/core/ipv4/icmp.c \
  $(LWIP)/src/core/ipv4/inet_chksum.c \
  $(LWIP)/src/core/dhcp.c \
  $(LWIP)/src/core/dns.c \
  $(LWIP)/src/api/tcpip.c \
  $(LWIP)/src/api/api_msg.c \
  $(LWIP)/src/api/err.c \
  $(LWIP)/src/api/netbuf.c \
  $(LWIP)/src/api/api_lib.c \
  $(LWIP)/src/netif/etharp.c \
  $(LWIP)/contrib/port/FreeRTOS/AT91SAM7X/sys_arch.c \
  $(LWIP)/src/netif/ethernetif.c \
  $(MAKINGTHINGS)/SAM7_EMAC.c \
  $(LWIP)/src/core/udp.c \
  $(LWIP)/src/core/ipv4/ip_frag.c \
  $(USB)/device/cdc-serial/CDCDSerialDriver.c \
  $(USB)/device/cdc-serial/CDCDSerialDriverDescriptors.c \
  $(USB)/device/core/USBDCallbacks_Initialized.c \
  $(USB)/device/core/USBDCallbacks_Reset.c \
  $(USB)/device/core/USBDCallbacks_Resumed.c \
  $(USB)/device/core/USBDCallbacks_Suspended.c \
  $(USB)/device/core/USBDDriverCallbacks_ConfigurationChanged.c \
  $(USB)/device/core/USBDDriverCallbacks_InterfaceSettingChanged.c \
  $(USB)/device/core/USBDDriver.c \
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

CPP_THUMB_SRC = \
  ../../core/startup/extras_cpp.cpp \
  $(MAKINGTHINGS)/io_cpp.cpp \
  $(MAKINGTHINGS)/led_.cpp \
  $(MAKINGTHINGS)/tcpsocket.cpp \
  $(MAKINGTHINGS)/udpsocket.cpp \
  $(MAKINGTHINGS)/tcpserver.cpp \
  $(MAKINGTHINGS)/osc_cpp.cpp \
  $(MAKINGTHINGS)/osc_message.cpp \
  $(MAKINGTHINGS)/osc_pattern.cpp \
  $(MAKINGTHINGS)/rtos_.cpp \
  $(MAKINGTHINGS)/usb_.cpp \
  $(LIBRARIES)/appled/appled_.cpp \
  make_.cpp \

# Source files that must be built in ARM mode.
ARM_SRC= \
  $(FREERTOS)/portable/GCC/ARM7_AT91SAM7S/portISR.c \
  $(MAKINGTHINGS)/SAM7_EMAC_ISR.c \
  $(MAKINGTHINGS)/USBIsr.c \
  $(MAKINGTHINGS)/analogin_isr.c \
  $(MAKINGTHINGS)/fasttimer_isr.c \
  $(MAKINGTHINGS)/serial_isr.c \
  $(MAKINGTHINGS)/timer_isr.c \
  $(USB)/device/core/USBD_UDP.c
  
# CPP_ARM_SRC = 
  
# All the include directories need to be appended here
INCLUDEDIRS = \
-I.  \
-I$(LIBRARIES)/appled  \
-I$(LIBRARIES)/digitalin  \
-I$(LIBRARIES)/digitalout  \
-I$(LIBRARIES)/dipswitch  \
-I$(LIBRARIES)/motor  \
-I$(LIBRARIES)/pwmout  \
-I$(LIBRARIES)/servo  \
-I$(LIBRARIES)/stepper  \
-I$(LIBRARIES)/webserver  \
-I$(LIBRARIES)/xbee  \
-I$(MAKINGTHINGS)  \
-I$(MAKINGTHINGS)/testing  \
-I$(LWIP)/src/include  \
-I$(LWIP)/contrib/port/FreeRTOS/AT91SAM7X  \
-I$(FREERTOS)/include  \
-I$(FREERTOS)/portable/GCC/ARM7_AT91SAM7S  \
-I$(LWIP)/src/include/ipv4 \
-I$(USB)/device/cdc-serial \
-I$(USB)/device/core \
-I$(USB)/common/core \
-I$(USB)/common/cdc

############################################################################
# Below here should not change from project to project

CC=arm-elf-gcc
CPP=arm-elf-g++
OBJCOPY=arm-elf-objcopy
ARCH=arm-elf-ar
CRT0=../../core/startup/AT91SAM7_Startup.s ../../core/startup/crt0.s
DEBUG=
OPTIM=-O2
LDSCRIPT=../../core/startup/make-controller.ld

# flags common to both c and c++
C_AND_CPP_FLAGS = \
$(INCLUDEDIRS) \
-Wall  \
-Wextra  \
-Wno-strict-aliasing  \
-fno-builtin-printf \
-D SAM7_GCC  \
-D THUMB_INTERWORK \
-D__FLASH_BUILD \
-DSUPERVISOR_START \
-mthumb-interwork \
-mcpu=arm7tdmi  \
-T$(LDSCRIPT) \
$(DEBUG)  \
$(OPTIM)

CFLAGS = ${C_AND_CPP_FLAGS}
CFLAGS += -Wmissing-prototypes -Wmissing-declarations

CPPFLAGS = ${C_AND_CPP_FLAGS}
CPPFLAGS += -fno-rtti -fno-exceptions -fno-unwind-tables

THUMB_FLAGS=-mthumb
LINKER_FLAGS=-Xlinker -ooutput/$(OUTPUT).elf -Xlinker -M -Xlinker -Map=output/$(OUTPUT)_o.map

ARM_OBJ = $(ARM_SRC:.c=.o)
THUMB_OBJ = $(THUMB_SRC:.c=.o)
CPP_THUMB_OBJ = $(CPP_THUMB_SRC:.cpp=.o)
CPP_ARM_OBJ = $(CPP_ARM_SRC:.cpp=.o)

output/$(OUTPUT).bin : output/$(OUTPUT).elf
	$(OBJCOPY) output/$(OUTPUT).elf -O binary output/$(OUTPUT).bin
	
output/$(OUTPUT).elf : $(ARM_OBJ) $(THUMB_OBJ) $(CPP_ARM_OBJ) $(CPP_THUMB_OBJ) $(CRT0)
	$(CC) $(CFLAGS) $(ARM_OBJ) $(THUMB_OBJ) $(CPP_ARM_OBJ) $(CPP_THUMB_OBJ) -nostartfiles -x assembler-with-cpp $(CRT0) $(LINKER_FLAGS)

$(THUMB_OBJ) : %.o : %.c config.h
	$(CC) -c $(THUMB_FLAGS) $(CFLAGS) $< -o $@

$(CPP_THUMB_OBJ) : %.o : %.cpp config.h
	$(CPP) -c $(THUMB_FLAGS) $(CPPFLAGS) $< -o $@

$(ARM_OBJ) : %.o : %.c config.h
	$(CC) -c $(CFLAGS) $< -o $@

$(CPP_ARM_OBJ) : %.o : %.cpp config.h
	$(CPP) -c $(CPPFLAGS) $< -o $@

clean :
	rm -f $(ARM_OBJ)
	rm -f $(THUMB_OBJ)
	rm -f $(CPP_ARM_OBJ)
	rm -f $(CPP_THUMB_OBJ)
	rm -f output/$(OUTPUT).elf
	rm -f output/$(OUTPUT).bin
	rm -f output/$(OUTPUT)_o.map


