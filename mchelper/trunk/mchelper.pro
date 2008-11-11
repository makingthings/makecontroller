MCHELPER_VERSION = "2.5.0"
TEMPLATE = app
TARGET = mchelper
CONFIG += qt debug
#CONFIG += qt release
#CONFIG -= debug

FORMS = layouts/mainwindow.ui \
        layouts/inspector.ui \
        layouts/preferences.ui \
        layouts/uploader.ui

HEADERS = include/MainWindow.h \
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
DEFINES += MCHELPER_VERSION=\\\"$${MCHELPER_VERSION}\\\"
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
unix{ 
  !macx{
    CONFIG += link_pkgconfig
    PKGCONFIG += dbus-1 hal
  }
}

win32:HEADERS += src/qextserialport/win_qextserialport.h
win32:SOURCES += src/qextserialport/win_qextserialport.cpp
win32:DEFINES += _TTY_WIN_
win32:DEFINES += WINVER=0x0501
win32:LIBS += -lSetupapi
win32:debug:CONFIG += console


# *******************************************
#              test suite
# *******************************************

test_suite {
  TARGET       = mchelper_test
  DEFINES     += MCHELPER_TEST_SUITE
  CONFIG      += qtestlib
  CONFIG      -= release # always, no matter what it's set to above
  DESTDIR      = tests
  INCLUDEPATH += tests
  
  SOURCES -=  src/main.cpp
  
  SOURCES +=  tests/main.cpp \
              tests/TestOsc.cpp \
              tests/TestXmlServer.cpp
              
  HEADERS +=  tests/TestOsc.h \
              tests/TestXmlServer.h
}



