#
# MAKINGTHINGS MAKE FILE FOR THE MAKE CONTROLLER
# Each project may be named uniquely 
# (See Output variable below) and possibly have different source files
# Dependencies are automatically created by the compiler (into .d files) and 
# included in the makefile 
#

# Name the output files here
OUTPUT = solo

# this rule says that when "make" is typed the bin file is created 		       
all: output/$(OUTPUT).bin

# Source files that can be built in THUMB mode.
THUMB_SRC= \
  make.c \
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
  ../../core/makingthings/io.c \
  ../../core/makingthings/led.c \
  ../../core/makingthings/timer.c \
  ../../core/makingthings/fasttimer.c \
  ../../core/makingthings/debugosc.c \
  ../../core/makingthings/can.c \
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
  
# Source files that must be built in ARM mode.
ARM_SRC= \
  ../../core/freertos/portable/GCC/ARM7_AT91SAM7S/portISR.c \
  ../../core/makingthings/SAM7_EMAC_ISR.c \
  ../../core/makingthings/USBIsr.c \
  ../../core/startup/Cstartup_SAM7.c \
  ../../core/makingthings/analogin_isr.c \
  ../../core/makingthings/fasttimer_isr.c \
  ../../core/makingthings/serial_isr.c \
  ../../core/makingthings/timer_isr.c \
  ../../core/makingthings/can_isr.c
  
# All the include directories need to be appended here
INCLUDEDIRS = \
-I.  \
-I../../core/makingthings  \
-I../../core/lwip/src/include  \
-I../../core/lwip/contrib/port/FreeRTOS/AT91SAM7X  \
-I../../core/freertos/include  \
-I../../core/freertos/portable/GCC/ARM7_AT91SAM7S  \
-I../../core/lwip/src/include/ipv4 \

############################################################################
# Below here should not change from project to project

CC=arm-elf-gcc
OBJCOPY=arm-elf-objcopy
ARCH=arm-elf-ar
CRT0=../../core/startup/boot.s
DEBUG=
OPTIM=-O2
LDSCRIPT=../../core/startup/atmel-rom.ld

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
-mthumb-interwork \
-mcpu=arm7tdmi  \
-T$(LDSCRIPT) \
$(DEBUG)  \
$(OPTIM)

THUMB_FLAGS=-mthumb
LINKER_FLAGS=-Xlinker -ooutput/$(OUTPUT).elf -Xlinker -M -Xlinker -Map=output/$(OUTPUT)_o.map

ARM_OBJ = $(ARM_SRC:.c=.o)
THUMB_OBJ = $(THUMB_SRC:.c=.o)

output/$(OUTPUT).bin : output/$(OUTPUT).elf
	$(OBJCOPY) output/$(OUTPUT).elf -O binary output/$(OUTPUT).bin
	
output/$(OUTPUT).elf : $(ARM_OBJ) $(THUMB_OBJ) $(CRT0)
	$(CC) $(CFLAGS) $(ARM_OBJ) $(THUMB_OBJ) -nostartfiles $(CRT0) $(LINKER_FLAGS)

$(THUMB_OBJ) : %.o : %.c
	$(CC) -c $(THUMB_FLAGS) $(CFLAGS) $< -o $@

$(ARM_OBJ) : %.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@
  	    
clean :
	rm -f $(ARM_OBJ)
	rm -f $(THUMB_OBJ)
	rm -f output/$(OUTPUT).elf     
	rm -f output/$(OUTPUT).bin     
	rm -f output/$(OUTPUT)_o.map     

