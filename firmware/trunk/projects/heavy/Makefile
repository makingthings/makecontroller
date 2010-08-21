
PROJECT = heavy
# available optimization levels: -O0, -O1, -O2, -O3, -Os
OPTIMIZATION = -O2

# components
LWIP      = ../../core/lwip
USB       = ../../core
CHIBIOS   = ../../core/chibios
MT        = ../../core/makingthings
LIBRARIES = ../../libraries

# Imported source files
include $(CHIBIOS)/os/ports/GCC/ARM7/port.mk
include $(CHIBIOS)/os/hal/platforms/AT91SAM7/platform.mk
include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/kernel/kernel.mk
include $(LWIP)/lwip.mk
include $(USB)/usb/usb.mk
include $(MT)/mtcore.mk

# C sources
CSRC = $(PORTSRC) $(KERNSRC) $(HALSRC) $(PLATFORMSRC) $(MTCORESRC) \
       $(USBCORESRC) $(USBCDCSRC) \
       $(LWNETIFSRC) $(LWCORESRC) $(LWIPV4SRC) $(LWAPISRC) \
       ${LWIP}/contrib/chibios/lwipthread.c \
       ${LWIP}/contrib/chibios/arch/sys_arch.c \
       $(CHIBIOS)/os/various/syscalls.c \
       $(CHIBIOS)/os/various/evtimer.c \
       $(LIBRARIES)/appled/appled.c \
       $(LIBRARIES)/digitalin/digitalin.c \
       $(LIBRARIES)/digitalout/digitalout.c \
       $(LIBRARIES)/dipswitch/dipswitch.c \
       $(LIBRARIES)/motor/motor.c \
       $(LIBRARIES)/pwmout/pwmout.c \
       $(PROJECT).c

# C++ sources
CPPSRC =

# include directories
INCDIR = $(PORTINC) $(KERNINC) $(HALINC) $(PLATFORMINC) \
				 $(MT) $(LWINC) $(USBINC) \
         $(CHIBIOS)/os/various \
         $(CHIBIOS)/os/ports/GCC/ARM7/AT91SAM7 \
         $(LIBRARIES)/appled \
         $(LIBRARIES)/digitalin \
         $(LIBRARIES)/digitalout \
         $(LIBRARIES)/dipswitch \
         $(LIBRARIES)/motor \
         $(LIBRARIES)/pwmout

# where to put the build output 
BUILDDIR = build

# assembly sources
ASMSRC = $(PORTASM) \
         $(CHIBIOS)/os/ports/GCC/ARM7/AT91SAM7/vectors.s

##############################################################################
# Compiler settings

MCU  = arm7tdmi

TRGT = arm-none-eabi-
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
# Enable loading with g++ only if you need C++ runtime support.
# NOTE: You can use C++ even without C++ support if you are careful. C++
#       runtime support makes code size explode.
LD   = $(TRGT)gcc
#LD   = $(TRGT)g++
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
OD   = $(TRGT)objdump
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary

# linker script
LDSCRIPT = $(MT)/ch.ld

# ARM-specific options here
AOPT =

# THUMB-specific options here
TOPT = -mthumb -DTHUMB

# Define C warning options here
CWARN = -Wall -Wextra -Wstrict-prototypes

# Define C++ warning options here
CPPWARN = -Wall -Wextra

#
# Compiler settings
##############################################################################

##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = ${OPTIMIZATION} -ggdb -fomit-frame-pointer -mabi=apcs-gnu
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti -fno-exceptions
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# If enabled, this option allows to compile the application in THUMB mode.
ifeq ($(USE_THUMB),)
  USE_THUMB = no
endif

# Enable register caching optimization (read documentation).
ifeq ($(USE_CURRP_CACHING),)
  USE_CURRP_CACHING = no
endif

#
# Build global options
##############################################################################

include $(CHIBIOS)/os/ports/GCC/ARM/rules.mk
