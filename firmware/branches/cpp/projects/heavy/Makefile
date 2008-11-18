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

# Source files that can be built in THUMB mode.
THUMB_SRC= \
  ../../core/freertos/tasks.c \
  ../../core/freertos/queue.c \
  ../../core/freertos/list.c \
  ../../core/freertos/portable/GCC/ARM7_AT91SAM7S/port.c \
  ../../core/startup/extras.c \
  ../../core/freertos/portable/MemMang/heap_2.c \
  ../../core/makingthings/main.c \
  ../../core/makingthings/eeprom.c \
  ../../core/makingthings/analogin.c \
  ../../core/makingthings/serial.c \
  ../../core/makingthings/pwm.c \
  ../../core/makingthings/spi.c \
  ../../core/makingthings/USB-CDC.c \
  ../../core/makingthings/rtos.c \
  ../../core/makingthings/usb.c \
  ../../core/makingthings/network.c \
  ../../core/makingthings/system.c \
  ../../libraries/xbee/xbee.c \
  ../../libraries/servo/servo.c \
  ../../libraries/stepper/stepper.c \
  ../../libraries/dipswitch/dipswitch.c \
  ../../libraries/motor/motor.c \
  ../../libraries/pwmout/pwmout.c \
  ../../libraries/digitalin/digitalin.c \
  ../../libraries/digitalout/digitalout.c \
  ../../libraries/webserver/webserver.c \
  ../../core/makingthings/io.c \
  ../../core/makingthings/timer.c \
  ../../core/makingthings/fasttimer.c \
  ../../core/makingthings/debugosc.c \
  ../../core/lwip/src/core/tcp_out.c \
  ../../core/lwip/src/core/ipv4/inet.c \
  ../../core/lwip/src/core/mem.c \
  ../../core/lwip/src/core/memp.c \
  ../../core/lwip/src/core/netif.c \
  ../../core/lwip/src/core/pbuf.c \
  ../../core/lwip/src/core/raw.c \
  ../../core/lwip/src/core/stats.c \
  ../../core/lwip/src/core/sys.c \
  ../../core/lwip/src/core/tcp.c \
  ../../core/lwip/src/core/tcp_in.c \
  ../../core/lwip/src/core/init.c \
  ../../core/lwip/src/core/ipv4/ip.c \
  ../../core/lwip/src/core/ipv4/ip_addr.c \
  ../../core/lwip/src/core/ipv4/icmp.c \
  ../../core/lwip/src/core/ipv4/inet_chksum.c \
  ../../core/lwip/src/core/dhcp.c \
  ../../core/lwip/src/core/dns.c \
  ../../core/lwip/src/api/tcpip.c \
  ../../core/lwip/src/api/api_msg.c \
  ../../core/lwip/src/api/err.c \
  ../../core/lwip/src/api/netbuf.c \
  ../../core/lwip/src/api/api_lib.c \
  ../../core/lwip/src/netif/etharp.c \
  ../../core/lwip/contrib/port/FreeRTOS/AT91SAM7X/sys_arch.c \
  ../../core/lwip/src/netif/ethernetif.c \
  ../../core/makingthings/SAM7_EMAC.c \
  ../../core/lwip/src/core/udp.c \
  ../../core/lwip/src/core/ipv4/ip_frag.c \
  ../../core/makingthings/osc.c \
  ../../core/makingthings/osc_patternmatch.c

CPP_THUMB_SRC = \
  ../../core/startup/extras_cpp.cpp \
  ../../core/makingthings/io_cpp.cpp \
  ../../core/makingthings/led_.cpp \
  ../../core/makingthings/tcpsocket.cpp \
  ../../core/makingthings/udpsocket.cpp \
  ../../core/makingthings/tcpserver.cpp \
  ../../core/makingthings/osc_cpp.cpp \
  ../../core/makingthings/rtos_.cpp \
  ../../libraries/appled/appled_.cpp \
  make_.cpp \

# Source files that must be built in ARM mode.
ARM_SRC= \
  ../../core/freertos/portable/GCC/ARM7_AT91SAM7S/portISR.c \
  ../../core/makingthings/SAM7_EMAC_ISR.c \
  ../../core/makingthings/USBIsr.c \
  ../../core/makingthings/analogin_isr.c \
  ../../core/makingthings/fasttimer_isr.c \
  ../../core/makingthings/serial_isr.c \
  ../../core/makingthings/timer_isr.c \
  
# CPP_ARM_SRC = \
# # ../../core/makingthings/analogin_isr_cpp.cpp \
  
# All the include directories need to be appended here
INCLUDEDIRS = \
-I.  \
-I../../libraries/appled  \
-I../../libraries/digitalin  \
-I../../libraries/digitalout  \
-I../../libraries/dipswitch  \
-I../../libraries/motor  \
-I../../libraries/pwmout  \
-I../../libraries/servo  \
-I../../libraries/stepper  \
-I../../libraries/webserver  \
-I../../libraries/xbee  \
-I../../core/makingthings  \
-I../../core/makingthings/testing  \
-I../../core/lwip/src/include  \
-I../../core/lwip/contrib/port/FreeRTOS/AT91SAM7X  \
-I../../core/freertos/include  \
-I../../core/freertos/portable/GCC/ARM7_AT91SAM7S  \
-I../../core/lwip/src/include/ipv4 \

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
CFLAGS += -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations

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


