#
# MakingThings -  Makefile for the Make Controller
#

# the name for your project...will result in PROJECT_NAME.bin
PROJECT_NAME = heavy
# set DEBUG to -g to include debug info, otherwise leave blank
# DEBUG = -g
DEBUG = 
# available optimization levels: -O0, -O1, -O2, -O3, -Os
OPTIMIZATION = -O3

FREERTOS = ../../core/freertos
LWIP = ../../core/lwip
USB = ../../core/usb
MAKINGTHINGS = ../../core/makingthings
LIBRARIES = ../../libraries
STARTUP = ../../core/startup

# Source files that can be built in THUMB mode.
THUMB_SRC= \
  make.c \
  $(FREERTOS)/tasks.c \
  $(FREERTOS)/queue.c \
  $(FREERTOS)/list.c \
  $(FREERTOS)/portable/GCC/ARM7_AT91SAM7S/port.c \
  $(STARTUP)/extras.c \
  $(FREERTOS)/portable/MemMang/heap_2.c \
  $(MAKINGTHINGS)/main.c \
  $(MAKINGTHINGS)/eeprom.c \
  $(MAKINGTHINGS)/analogin.c \
  $(MAKINGTHINGS)/serial.c \
  $(MAKINGTHINGS)/pwm.c \
  $(MAKINGTHINGS)/spi.c \
  $(MAKINGTHINGS)/usb_serial.c \
  $(MAKINGTHINGS)/rtos.c \
  $(MAKINGTHINGS)/network.c \
  $(MAKINGTHINGS)/system.c \
  $(LIBRARIES)/xbee/xbee.c \
  $(LIBRARIES)/servo/servo.c \
  $(LIBRARIES)/stepper/stepper.c \
  $(LIBRARIES)/dipswitch/dipswitch.c \
  $(LIBRARIES)/motor/motor.c \
  $(LIBRARIES)/appled/appled.c \
  $(LIBRARIES)/pwmout/pwmout.c \
  $(LIBRARIES)/digitalin/digitalin.c \
  $(LIBRARIES)/digitalout/digitalout.c \
  $(LIBRARIES)/webserver/webserver.c \
  $(MAKINGTHINGS)/io.c \
  $(MAKINGTHINGS)/led.c \
  $(MAKINGTHINGS)/timer.c \
  $(MAKINGTHINGS)/fasttimer.c \
  $(MAKINGTHINGS)/debugosc.c \
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
  $(MAKINGTHINGS)/osc.c \
  $(MAKINGTHINGS)/osc_patternmatch.c \
  $(USB)/device/cdc-serial/CDCDSerialDriver.c \
  $(USB)/device/cdc-serial/CDCDSerialDriverDescriptors.c \
  $(USB)/device/core/USBDCallbacks_Initialized.c \
  $(USB)/device/core/USBDCallbacks_Reset.c \
  $(USB)/device/core/USBDCallbacks_Resumed.c \
  $(USB)/device/core/USBDCallbacks_Suspended.c \
  $(USB)/device/core/USBDDriverCb_CfgChanged.c \
  $(USB)/device/core/USBDDriverCb_IfSettingChanged.c \
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
  
# Source files that must be built in ARM mode.
ARM_SRC= \
  $(FREERTOS)/portable/GCC/ARM7_AT91SAM7S/portISR.c \
  $(MAKINGTHINGS)/SAM7_EMAC_ISR.c \
  $(MAKINGTHINGS)/analogin_isr.c \
  $(MAKINGTHINGS)/fasttimer_isr.c \
  $(MAKINGTHINGS)/serial_isr.c \
  $(MAKINGTHINGS)/timer_isr.c \
  $(MAKINGTHINGS)/can_isr.c \
  $(USB)/device/core/USBD_UDP.c
  
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
OBJCOPY=arm-elf-objcopy
ARCH=arm-elf-ar
CRT0=$(STARTUP)/AT91SAM7_Startup.s $(STARTUP)/crt0.s
LDSCRIPT=$(STARTUP)/make-controller.ld

#
# CFLAGS common to both the THUMB and ARM mode builds
#

CFLAGS= \
$(INCLUDEDIRS) \
-Wall  \
-Wextra  \
-Wstrict-prototypes  \
-Wmissing-prototypes  \
-Wmissing-declarations  \
-Wno-strict-aliasing  \
-D SAM7_GCC  \
-D THUMB_INTERWORK \
-D__FLASH_BUILD \
-DSUPERVISOR_START \
-mthumb-interwork \
-mcpu=arm7tdmi  \
-T$(LDSCRIPT) \
$(DEBUG)  \
$(OPTIMIZATION)

THUMB_FLAGS=-mthumb
LINKER_FLAGS=-Xlinker -ooutput/$(PROJECT_NAME).elf -Xlinker -M -Xlinker -Map=output/$(PROJECT_NAME)_o.map

ARM_OBJ = $(ARM_SRC:.c=.o)
THUMB_OBJ = $(THUMB_SRC:.c=.o)

all: output/$(PROJECT_NAME).bin

output/$(PROJECT_NAME).bin : output/$(PROJECT_NAME).elf
	$(OBJCOPY) output/$(PROJECT_NAME).elf -O binary output/$(PROJECT_NAME).bin
	
output/$(PROJECT_NAME).elf : $(ARM_OBJ) $(THUMB_OBJ) $(CRT0)
	$(CC) $(CFLAGS) $(ARM_OBJ) $(THUMB_OBJ) -nostartfiles -x assembler-with-cpp $(CRT0) $(LINKER_FLAGS)

$(THUMB_OBJ) : %.o : %.c config.h
	$(CC) -c $(THUMB_FLAGS) $(CFLAGS) $< -o $@

$(ARM_OBJ) : %.o : %.c config.h
	$(CC) -c $(CFLAGS) $< -o $@
  	    
clean :
	rm -f $(ARM_OBJ)
	rm -f $(THUMB_OBJ)
	rm -f output/$(PROJECT_NAME).elf     
	rm -f output/$(PROJECT_NAME).bin     
	rm -f output/$(PROJECT_NAME)_o.map     


