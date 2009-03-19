#
# MAKINGTHINGS MAKE FILE FOR THE MAKE CONTROLLER
# Each project may be named uniquely 
# (See Output variable below) and possibly have different source files
# Dependencies are automatically created by the compiler (into .d files) and 
# included in the makefile 
#

# Name the output files here
OUTPUT = tiny

# this rule says that when "make" is typed the bin file is created 		       
all: output/$(OUTPUT).bin

#
# Source files that can be built to THUMB mode.
#
THUMB_SRC=  \
  main.c \
  ../../core/makingthings/io.c \
  ../../core/makingthings/led.c \

# Source files that must be built to ARM mode.
ARM_SRC= \
  ../../core/startup/Cstartup_SAM7.c

# All the include directories need to be appended here
INCLUDEDIRS = \
-I.  \
-I../../core/makingthings  \
-I../../core/freertos/portable/GCC/ARM7_AT91SAM7S  \

############################################################################
# Below here should not change from project to project

CC=arm-elf-gcc
OBJCOPY=arm-elf-objcopy
ARCH=arm-elf-ar
CRT0=../../core/startup/boot.s
DEBUG=
#OPTIM=-Os
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
LINKER_FLAGS=-Xlinker -ooutput/$(OUTPUT).elf -Xlinker -M -Xlinker -Map=output/$(OUTPUT).map

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
	rm -f output/$(OUTPUT).map     

