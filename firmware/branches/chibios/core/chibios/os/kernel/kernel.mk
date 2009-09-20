# List of all the ChibiOS/RT kernel files, there is no need to remove the files
# from this list, you can disable parts of the kernel by editing chconf.h.
KERNSRC = ${CHIBIOS}/kernel/src/chsys.c       ${CHIBIOS}/kernel/src/chdebug.c \
          ${CHIBIOS}/kernel/src/chlists.c     ${CHIBIOS}/kernel/src/chvt.c \
          ${CHIBIOS}/kernel/src/chschd.c      ${CHIBIOS}/kernel/src/chthreads.c \
          ${CHIBIOS}/kernel/src/chsem.c       ${CHIBIOS}/kernel/src/chmtx.c \
          ${CHIBIOS}/kernel/src/chcond.c      ${CHIBIOS}/kernel/src/chevents.c \
          ${CHIBIOS}/kernel/src/chmsg.c       ${CHIBIOS}/kernel/src/chmboxes.c \
          ${CHIBIOS}/kernel/src/chqueues.c    ${CHIBIOS}/kernel/src/chheap.c \
          ${CHIBIOS}/kernel/src/chmempools.c

# Required include directories
KERNINC = ${CHIBIOS}/kernel/include
