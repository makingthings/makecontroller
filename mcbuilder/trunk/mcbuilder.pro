TEMPLATE = app

MCBUILDER_VERSION = "0.1.0"

FORMS = layouts/mainwindow.ui \
        layouts/preferences.ui \
        layouts/properties.ui \
        layouts/usbmonitor.ui \
        layouts/findreplace.ui

HEADERS = include/Highlighter.h \
          include/MainWindow.h \
          include/Preferences.h \
          include/Uploader.h \
          include/Properties.h \
          include/Builder.h \
          include/UsbMonitor.h \
          include/FindReplace.h \
          include/About.h \
          include/ConsoleItem.h

SOURCES = src/main.cpp \
          src/Highlighter.cpp \
          src/MainWindow.cpp \
          src/Preferences.cpp \
          src/Uploader.cpp \
          src/Properties.cpp \
          src/Builder.cpp \
          src/UsbMonitor.cpp \
          src/FindReplace.cpp \
          src/About.cpp

TARGET = mcbuilder

QT += xml network
INCLUDEPATH += include
RESOURCES   += resources/icons/icons_rsrc.qrc
DEFINES     += MCBUILDER_VERSION=\\\"$${MCBUILDER_VERSION}\\\"
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
unix{ !macx: LIBS += -lusb } # use libusb on other unices

win32{
	HEADERS += src/qextserialport/win_qextserialport.h
	SOURCES += src/qextserialport/win_qextserialport.cpp
	DEFINES += _TTY_WIN_
	LIBS += -lSetupapi	
}






