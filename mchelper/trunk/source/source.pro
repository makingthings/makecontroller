MCHELPER_VERSION = "2.5.1"
TEMPLATE = app
TARGET = mchelper

FORMS = ../layouts/mainwindow.ui \
        ../layouts/inspector.ui \
        ../layouts/preferences.ui \
        ../layouts/uploader.ui \
        ../layouts/about.ui

HEADERS = MainWindow.h \
          OscXmlServer.h \
          Osc.h \
          Inspector.h \
          NetworkMonitor.h \
          UsbMonitor.h \
          Preferences.h \
          Uploader.h \
          About.h \
          Board.h \
          PacketInterface.h \
          MsgType.h \
          BoardType.h \
          PacketUdp.h \
          PacketUsbSerial.h \
          AppUpdater.h

SOURCES = main.cpp \
          MainWindow.cpp \
          OscXmlServer.cpp \
          Osc.cpp \
          Inspector.cpp \
          NetworkMonitor.cpp \
          UsbMonitor.cpp \
          Preferences.cpp \
          Uploader.cpp \
          About.cpp \
          Board.cpp \
          PacketUdp.cpp \
          PacketUsbSerial.cpp \
          AppUpdater.cpp

TRANSLATIONS = translations/mchelper_fr.ts

QT += network xml
DEFINES += MCHELPER_VERSION=\\\"$${MCHELPER_VERSION}\\\"
RESOURCES   += resources/mchelper.qrc
OBJECTS_DIR  = tmp
MOC_DIR      = tmp
RCC_DIR      = tmp
UI_DIR       = tmp

# *******************************************
#           platform specific stuff
# *******************************************
macx{
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3
  QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk #need this if building on PPC
  ICON = resources/icons/mchelper.icns
}

win32{
  RC_FILE = resources/icons/mchelper.rc # for application icon
}

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
  
  SOURCES -=  source/main.cpp
  
  SOURCES +=  tests/main.cpp \
              tests/TestOsc.cpp \
              tests/TestXmlServer.cpp
              
  HEADERS +=  tests/TestOsc.h \
              tests/TestXmlServer.h
}



