

CONFIG += console qtestlib
TEMPLATE = app
INCLUDEPATH += ../include tmp
OBJECTS_DIR  = tmp
MOC_DIR      = tmp
RCC_DIR      = tmp
UI_DIR       = tmp

# Input
HEADERS += ../include/Osc.h \
            tmp/testmchelper.moc
            
SOURCES += testmchelper.cpp \
            ../src/Osc.cpp
