#
# MAKINGTHINGS MAKE FILE FOR THE MAKE CONTROLLER
# Each project may be named uniquely 
# (See Output variable below) and possibly have different source files
# Dependencies are automatically created by the compiler (into .d files) and 
# included in the makefile 
#

# Name the output files here
OUTPUT = testproj

# this rule says that when "make" is typed the bin file is created 		       
all: output/$(OUTPUT).bin

# Source files that can be built in THUMB mode.
THUMB_SRC= \
  ../controller/freertos/tasks.c \
  ../controller/freertos/queue.c \
  ../controller/freertos/list.c \
  ../controller/freertos/portable/GCC/ARM7_AT91SAM7S/port.c \
  ../controller/startup/extras.c \
  ../controller/freertos/portable/MemMang/heap_2.c \
  ../controller/makingthings/main.c \
  ../controller/makingthings/BasicWEB.c \
  make.c \
  ../controller/makingthings/eeprom.c \
  ../controller/makingthings/adc.c \
  ../controller/makingthings/pwm.c \
  ../controller/makingthings/spi.c \
  ../controller/makingthings/USB-CDC.c \
  ../controller/makingthings/rtos.c \
  ../controller/makingthings/socket.c \
  ../appboard/makingthings/servo.c \
  ../appboard/makingthings/dipswitch.c \
  ../appboard/makingthings/motor.c \
  ../appboard/makingthings/appboard.c \
  ../appboard/makingthings/appled.c \
  ../appboard/makingthings/pwmout.c \
  ../appboard/makingthings/digitalin.c \
  ../appboard/makingthings/digitalout.c \
  ../controller/makingthings/controller.c \
  ../controller/makingthings/io.c \
  ../controller/makingthings/led.c \
  ../controller/makingthings/timer.c \
  ../controller/makingthings/debugosc.c \
  ../controller/makingthings/can.c \
  ../controller/lwip/src/core/tcp_out.c \
  ../controller/lwip/src/core/inet.c \
  ../controller/lwip/src/core/mem.c \
  ../controller/lwip/src/core/memp.c \
  ../controller/lwip/src/core/netif.c \
  ../controller/lwip/src/core/pbuf.c \
  ../controller/lwip/src/core/raw.c \
  ../controller/lwip/src/core/stats.c \
  ../controller/lwip/src/core/sys.c \
  ../controller/lwip/src/core/tcp.c \
  ../controller/lwip/src/core/tcp_in.c \
  ../controller/lwip/src/core/ipv4/ip.c \
  ../controller/lwip/src/core/ipv4/ip_addr.c \
  ../controller/lwip/src/core/ipv4/icmp.c \
  ../controller/lwip/src/api/tcpip.c \
  ../controller/lwip/src/api/api_msg.c \
  ../controller/lwip/src/api/err.c \
  ../controller/lwip/src/api/api_lib.c \
  ../controller/lwip/src/netif/etharp.c \
  ../controller/lwip/contrib/port/FreeRTOS/AT91SAM7X/sys_arch.c \
  ../controller/lwip/src/netif/ethernetif.c \
  ../controller/makingthings/SAM7_EMAC.c \
  ../controller/lwip/src/core/udp.c \
  ../controller/lwip/src/core/ipv4/ip_frag.c \
  ../utility/makingthings/protocol.c \
  ../utility/makingthings/osc.c \
  ../utility/makingthings/osc_patternmatch.c
  
# Source files that must be built in ARM mode.
ARM_SRC= \
  ../controller/freertos/portable/GCC/ARM7_AT91SAM7S/portISR.c \
  ../controller/makingthings/SAM7_EMAC_ISR.c \
  ../controller/makingthings/USBIsr.c \
  ../controller/startup/Cstartup_SAM7.c \
  ../controller/makingthings/adc_isr.c \
  ../controller/makingthings/controller_isr.c \
  ../controller/makingthings/timer_isr.c \
  ../controller/makingthings/can_isr.c
  
# All the include directories need to be appended here
INCLUDEDIRS = \
-I.  \
-I../appboard/makingthings  \
-I../utility/makingthings  \
-I../controller/makingthings  \
-I../controller/lwip/src/include  \
-I../controller/lwip/contrib/port/FreeRTOS/AT91SAM7X  \
-I../controller/freertos/include  \
-I../controller/freertos/portable/GCC/ARM7_AT91SAM7S  \
-I../controller/lwip/src/include/ipv4 \

############################################################################
# Below here should not change from project to project

CC=arm-elf-gcc
OBJCOPY=arm-elf-objcopy
ARCH=arm-elf-ar
CRT0=../controller/startup/boot.s
DEBUG=
# OPTIM=-Os
LDSCRIPT=../controller/startup/atmel-rom.ld

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
LINKER_FLAGS=-Xlinker -ooutput/$(OUTPUT).elf -Xlinker -M -Xlinker -Map=$(OUTPUT).map

ARM_OBJ = $(ARM_SRC:.c=.o)
THUMB_OBJ = $(THUMB_SRC:.c=.o)

%.d : %.c
	$(CC) -MM $(CFLAGS) $< >$@.dd 
	sed "s,\($*\)\.o[ :]*,\1.o $@ : ,g" <$@.dd >$@
	rm -f $@.dd
				    				    
include $(ARM_SRC:.c=.d)	
include $(THUMB_SRC:.c=.d)	

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
	rm -f $(ARM_SRC:.c=.d)	
	rm -f $(THUMB_SRC:.c=.d)   
	rm -f output/$(OUTPUT).elf     
	rm -f output/$(OUTPUT).bin     
	rm -f output/$(OUTPUT).map     

