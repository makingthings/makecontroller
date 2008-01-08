# ------------------------------------------------------------------------------
#
# Copyright 2006-2007 MakingThings
#
# Licensed under the Apache License, 
# Version 2.0 (the "License"); you may not use this file except in compliance 
# with the License. You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0 
# 
# Unless required by applicable law or agreed to in writing, software distributed
# under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, either express or implied. See the License for
# the specific language governing permissions and limitations under the License.
#
# ------------------------------------------------------------------------------

MCHELPER_VERSION = "2.1.0"
TEMPLATE = app
FORMS = layouts/mchelper.ui layouts/mchelperPrefs.ui
#CONFIG += qt release
CONFIG += qt debug warn_on

HEADERS = 	McHelperWindow.h \
			UploaderThread.h \
			PacketInterface.h \
			PacketReadyInterface.h \
			MonitorInterface.h \
			PacketUdp.h \
			Osc.h \
			Samba.h \
			UsbSerial.h \
			PacketUsbCdc.h \
			UsbMonitor.h \
			NetworkMonitor.h \
			SambaMonitor.h \
			Board.h \
			MessageEvent.h \
			BoardArrivalEvent.h \
			OutputWindow.h \
			OscXmlServer.h \
			AppUpdater.h \
			McHelperPrefs.h
				
            
SOURCES	= 	main.cpp \
			McHelperWindow.cpp \
			UploaderThread.cpp \
			PacketUdp.cpp \
			Osc.cpp \
			Samba.cpp \
			UsbSerial.cpp \
			PacketUsbCdc.cpp \
			UsbMonitor.cpp \
			NetworkMonitor.cpp \
			SambaMonitor.cpp \
			Board.cpp \
			MessageEvent.cpp \
			OutputWindow.cpp \
			OscXmlServer.cpp \
			AppUpdater.cpp \
			McHelperPrefs.cpp
				
TARGET = mchelper
            
QT += network xml
RESOURCES     += resources/mchelper.qrc
DEFINES     += MCHELPER_VERSION=\"$${MCHELPER_VERSION}\"
OBJECTS_DIR  = tmp
MOC_DIR      = tmp
DESTDIR      = bin

QTDIR_build:REQUIRES="contains(QT_CONFIG, small-config)"

# ------------------------------------------------------------------
# qextserialport stuff
# ------------------------------------------------------------------

INCLUDEPATH += qextserialport
HEADERS	+= qextserialport/qextserialbase.h qextserialport/qextserialport.h
SOURCES	+= qextserialport/qextserialbase.cpp qextserialport/qextserialport.cpp

unix:HEADERS	+= qextserialport/posix_qextserialport.h
unix:SOURCES	+= qextserialport/posix_qextserialport.cpp
unix:DEFINES	+= _TTY_POSIX_

win32:HEADERS	+= qextserialport/win_qextserialport.h
win32:SOURCES	+= qextserialport/win_qextserialport.cpp
win32:DEFINES	+= _TTY_WIN_

# ------------------------------------------------------------------
# platform specific stuff
# ------------------------------------------------------------------

macx{
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3
	QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk #need this if building on PPC

	LIBS += -framework IOKit
	#CONFIG += x86 ppc
  ICON = resources/mchelper.icns
  QMAKE_INFO_PLIST = resources/osx/Info.plist
	QMAKE_POST_LINK = "sed -e s/@@version@@/$${MCHELPER_VERSION}/g -i '' bin/mchelper.app/Contents/Info.plist" && \
                      strip bin/mchelper.app/Contents/MacOS/mchelper
}


win32{
  LIBS += -lSetupapi
  RC_FILE = resources/mchelper.rc # for application icon
  DEFINES += WINVER=0x0501 _TTY_WIN_ QWT_DLL QT_DLL
  
  # If in debug mode, let's show the output of any
  # q[Debug|Warning|Critical|Fatal]() calls to the console
  #debug {
    #CONFIG += console
  #}
}





