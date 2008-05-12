TEMPLATE = app
TARGET = mchelper
# CONFIG += qt release
CONFIG += qt debug
MCHELPER_VERSION = "2.5.0"

FORMS = layouts/mainwindow.ui \
        layouts/inspector.ui \
        layouts/preferences.ui

HEADERS = include/MainWindow.h \
          src/bonjour/BonjourRecord.h \
          src/bonjour/BonjourServiceBrowser.h \
          src/bonjour/BonjourServiceRegister.h \
          src/bonjour/BonjourServiceResolver.h \
          include/OscXmlServer.h \
          include/Osc.h \
          include/Inspector.h \
          include/NetworkMonitor.h \
          include/UsbMonitor.h \
          include/Preferences.h \
          include/Uploader.h \
          include/About.h \
          include/Board.h \
          include/PacketInterface.h \
          include/MsgType.h \
          include/BoardType.h \
          include/PacketUdp.h \
          include/PacketUsbSerial.h

SOURCES = src/main.cpp \
          src/MainWindow.cpp \
          src/bonjour/BonjourServiceBrowser.cpp \
          src/bonjour/BonjourServiceRegister.cpp \
          src/bonjour/BonjourServiceResolver.cpp \
          src/OscXmlServer.cpp \
          src/Osc.cpp \
          src/Inspector.cpp \
          src/NetworkMonitor.cpp \
          src/UsbMonitor.cpp \
          src/Preferences.cpp \
          src/Uploader.cpp \
          src/About.cpp \
          src/Board.cpp \
          src/PacketUdp.cpp \
          src/PacketUsbSerial.cpp

QT += network xml
DEFINES     += MCHELPER_VERSION=\"$${MCHELPER_VERSION}\"
RESOURCES   += resources/mchelper.qrc
INCLUDEPATH += include
OBJECTS_DIR  = tmp
MOC_DIR      = tmp
RCC_DIR      = tmp
UI_DIR       = tmp
DESTDIR      = bin

# *******************************************
#           platform specific stuff
# *******************************************
macx{
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3
  QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk #need this if building on PPC
}

# OS X links Bonjour all by itself
!mac:LIBS += -ldns_sd

# *******************************************
#              qextserialport
# *******************************************
INCLUDEPATH += src/qextserialport
HEADERS +=  src/qextserialport/qextserialbase.h \
            src/qextserialport/qextserialport.h \
            src/qextserialport/qextserialenumerator.h
            
SOURCES +=  src/qextserialport/qextserialbase.cpp \
            src/qextserialport/qextserialport.cpp \
            src/qextserialport/qextserialenumerator.cpp

unix:HEADERS  += src/qextserialport/posix_qextserialport.h
unix:SOURCES  += src/qextserialport/posix_qextserialport.cpp
unix:DEFINES  += _TTY_POSIX_
macx: LIBS += -framework IOKit # use IOKit on OS X
unix{ !macx: LIBS += -lusb } # use libusb on other unices

win32:HEADERS += src/qextserialport/win_qextserialport.h
win32:SOURCES += src/qextserialport/win_qextserialport.cpp
win32:DEFINES += _TTY_WIN_



