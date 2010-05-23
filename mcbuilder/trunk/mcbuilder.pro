MCBUILDER_VERSION = "0.8.0"
TEMPLATE = app

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
  QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.5.sdk #need this if building on PPC
  release{ CONFIG += x86 }
  ICON = resources/icons/mcbuilder.icns
}

win32{
  RC_FILE = resources/icons/mcbuilder.rc
}

# *******************************************
#              qextserialport
# *******************************************
INCLUDEPATH += src/qextserialport
HEADERS +=  src/qextserialport/qextserialport.h \
            src/qextserialport/qextserialenumerator.h

SOURCES +=  src/qextserialport/qextserialport.cpp \
            src/qextserialport/qextserialenumerator.cpp

unix:SOURCES  += src/qextserialport/posix_qextserialport.cpp
macx: LIBS += -framework IOKit -framework Carbon
unix{
  !macx{
    CONFIG += link_pkgconfig
    PKGCONFIG += dbus-1 hal
  }
}

win32{
  SOURCES += src/qextserialport/win_qextserialport.cpp
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




