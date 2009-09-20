

##############################################################################
# Project, sources and paths

# Define project name here
PROJECT = chibitest
# available optimization levels: -O0, -O1, -O2, -O3, -Os
OPTIMIZATION = -Os
# set DEBUG to -g to include debug info, otherwise leave blank
DEBUG = 

#LWIP = ../../core/lwip
USB = ../../core/usb
CHIBIOS = ../../core/chibios
MT = ../../core/makingthings
MTLIBS = ../../libraries
STARTUP = ../../core/startup

# Define linker script file here
LDSCRIPT = ${STARTUP}/ch.ld

# Imported source files
include ${CHIBIOS}/os/ports/GCC/ARM7/port.mk
include ${CHIBIOS}/os/kernel/kernel.mk
include ${USB}/usb.mk

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = ${PORTSRC} \
       ${KERNSRC} \
			 ${USBSRC} \
       ${CHIBIOS}/os/io/pal.c \
       ${CHIBIOS}/os/io/serial.c \
       ${CHIBIOS}/os/io/platforms/AT91SAM7X/pal_lld.c \
       ${CHIBIOS}/os/io/platforms/AT91SAM7X/serial_lld.c \
       ${CHIBIOS}/os/io/platforms/AT91SAM7X/at91lib/aic.c \
			 ${MT}/analogin.c \
			 ${MT}/spi.c \
       board.c main.c

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC =

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACSRC =

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACPPSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCPPSRC =

# List ASM source files here
ASMSRC = $(PORTASM) \
         ${CHIBIOS}/os/ports/GCC/ARM7/AT91SAM7X/vectors.s

INCDIR = $(PORTINC) $(KERNINC) ${MT} ${USBINC} \
         ${CHIBIOS}/os/io \
         ${CHIBIOS}/os/io/platforms/AT91SAM7X \
         ${CHIBIOS}/os/various \
         ${CHIBIOS}/os/ports/GCC/ARM7/AT91SAM7X

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

MCU  = arm7tdmi

TRGT = arm-elf-
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

# ARM-specific options here
AOPT =

# THUMB-specific options here
TOPT = -mthumb -DTHUMB

# Define C warning options here
CWARN = -Wall -Wstrict-prototypes

# Define C++ warning options here
CPPWARN = -Wall

#
# Compiler settings
##############################################################################

##############################################################################
# Start of default section
#

# List all default C defines here, like -D_DEBUG=1
DDEFS =

# List all default ASM defines here, like -D_DEBUG=1
DADEFS =

# List all default directories to look for include files here
DINCDIR =

# List the default directory to look for the libraries here
DLIBDIR =

# List all default libraries here
DLIBS =

#
# End of default section
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
UDEFS =

# Define ASM defines here
UADEFS =

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

#
# End of user defines
##############################################################################

##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

# Compiler options here.
ifeq ($(USE_OPT),)
  USE_OPT = ${OPTIMIZATION} ${DEBUG} -ggdb -fomit-frame-pointer -mabi=apcs-gnu
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
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

include ${CHIBIOS}/os/ports/GCC/ARM/rules.mk
