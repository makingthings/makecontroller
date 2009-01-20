MCBUILDER_VERSION = "0.6.0"
TEMPLATE = app
CONFIG += debug
#CONFIG += release
CONFIG -= release

FORMS = layouts/mainwindow.ui \
        layouts/preferences.ui \
        layouts/projectinfo.ui \
        layouts/usbconsole.ui \
        layouts/findreplace.ui \
        layouts/about.ui \
        layouts/buildlog.ui

HEADERS = include/Highlighter.h \
          include/MainWindow.h \
          include/Preferences.h \
          include/Uploader.h \
          include/ProjectInfo.h \
          include/Builder.h \
          include/UsbConsole.h \
          include/FindReplace.h \
          include/About.h \
          include/AppUpdater.h \
          include/ConsoleItem.h \
          include/BuildLog.h \
          include/ProjectManager.h

SOURCES = src/main.cpp \
          src/Highlighter.cpp \
          src/MainWindow.cpp \
          src/Preferences.cpp \
          src/Uploader.cpp \
          src/ProjectInfo.cpp \
          src/Builder.cpp \
          src/UsbConsole.cpp \
          src/FindReplace.cpp \
          src/AppUpdater.cpp \
          src/About.cpp \
          src/BuildLog.cpp \
          src/ProjectManager.cpp
          
TRANSLATIONS = translations/mcbuilder_fr.ts

TARGET = mcbuilder

QT += xml network
INCLUDEPATH += include
RESOURCES   += resources/icons/icons_rsrc.qrc
DEFINES     += MCBUILDER_VERSION=\\\"$${MCBUILDER_VERSION}\\\"
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
  release{ CONFIG += x86 ppc }
  ICON = resources/icons/mcbuilder.icns
}

win32{
  RC_FILE = resources/icons/mcbuilder.rc
  debug{ CONFIG += console }
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
unix{ !macx: LIBS += -lusb } # use libusb on other unices

win32{
	HEADERS += src/qextserialport/win_qextserialport.h
	SOURCES += src/qextserialport/win_qextserialport.cpp
	DEFINES += _TTY_WIN_
	DEFINES += WINVER=0x0501
	LIBS += -lSetupapi
}


# *******************************************
#              test suite
# *******************************************

test_suite {
  TARGET       = mcbuilder_test
  DEFINES     += MCBUILDER_TEST_SUITE
  CONFIG      += qtestlib
  CONFIG      -= release # always, no matter what it's set to above
  DESTDIR      = tests
  INCLUDEPATH += tests
  
  SOURCES -=  src/main.cpp
  
  SOURCES +=  tests/main.cpp \
              tests/TestProjectManager.cpp \
              tests/TestBuilder.cpp \
              tests/TestProjectInfo.cpp \
              
  HEADERS +=  tests/TestProjectManager.h \
              tests/TestBuilder.h \
              tests/TestProjectInfo.h \
}




