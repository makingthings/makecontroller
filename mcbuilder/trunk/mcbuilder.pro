TEMPLATE = app
# CONFIG += qt release
CONFIG += qt debug

FORMS = layouts/mainwindow.ui \
        layouts/preferences.ui \
        layouts/properties.ui \
        layouts/serialmonitor.ui \
        layouts/findreplace.ui

HEADERS = include/Highlighter.h \
          include/MainWindow.h \
          include/Preferences.h \
          include/Uploader.h \
          include/ProjectProperties.h \
          include/Builder.h \
          include/SerialMonitor.h \
          include/FindReplace.h

SOURCES = src/main.cpp \
          src/Highlighter.cpp \
          src/MainWindow.cpp \
          src/Preferences.cpp \
          src/Uploader.cpp \
          src/ProjectProperties.cpp \
          src/Builder.cpp \
          src/SerialMonitor.cpp \
          src/FindReplace.cpp

TARGET = mcbuilder

QT += xml
INCLUDEPATH += include
OBJECTS_DIR  = tmp
MOC_DIR      = tmp
UI_DIR       = tmp
DESTDIR      = bin

# *******************************************
#           platform specific stuff
# *******************************************
macx{
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3
  QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk #need this if building on PPC
}
