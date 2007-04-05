\TEMPLATE	= app

FORMS = atest.ui

CONFIG		+= qt warn_on

HEADERS		= ATestWindow.h ATestThread.h ATestee.h PacketInterface.h PacketUdp.h Osc.h Samba.h
            
SOURCES		= main.cpp ATestWindow.cpp ATestThread.cpp ATestee.cpp PacketUdp.cpp Osc.cpp Samba.cpp
            
QT += network

QTDIR_build:REQUIRES="contains(QT_CONFIG, small-config)"

# ------------------------------------------------------------------
# use scopes to include the appropriate USB biz.

macx{
  message("This project is being built on a Mac.")
  LIBS += -framework IOKit
}


win32{
  message("This project is being built on Windows.")
  
  DEFINES += __LITTLE_ENDIAN__
  LIBS += -lSetupapi
}

unix{
  message("This project is being built on a Unix variant.")
}


