

CONFIG += console qtestlib
QT += xml
TEMPLATE = app
INCLUDEPATH += ../include ../tmp
OBJECTS_DIR  = ../tmp
MOC_DIR      = ../tmp
RCC_DIR      = ../tmp
UI_DIR       = ../tmp

# Input
HEADERS += ../include/ProjectManager.h \
            ../tmp/TestMcbuilder.moc
            
SOURCES += TestMcbuilder.cpp \
            ../src/ProjectManager.cpp
