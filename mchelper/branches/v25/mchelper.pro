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

MCHELPER_VERSION = "2.2.0"
TEMPLATE = app
FORMS = layouts/mchelper.ui layouts/mchelperPrefs.ui
#CONFIG += qt release
CONFIG += qt debug

HEADERS = 	include/McHelperWindow.h \
			include/UploaderThread.h \
			include/PacketInterface.h \
			include/PacketReadyInterface.h \
			include/MonitorInterface.h \
			include/PacketUdp.h \
			include/Osc.h \
			include/Samba.h \
			include/UsbSerial.h \
			include/PacketUsbCdc.h \
			include/UsbMonitor.h \
			include/NetworkMonitor.h \
			include/SambaMonitor.h \
			include/Board.h \
			include/MessageEvent.h \
			include/BoardArrivalEvent.h \
			include/OutputWindow.h \
			include/OscXmlServer.h \
			include/AppUpdater.h \
			include/McHelperPrefs.h
				
            
SOURCES	= 	source/main.cpp \
			source/McHelperWindow.cpp \
			source/UploaderThread.cpp \
			source/PacketUdp.cpp \
			source/Osc.cpp \
			source/Samba.cpp \
			source/UsbSerial.cpp \
			source/PacketUsbCdc.cpp \
			source/UsbMonitor.cpp \
			source/NetworkMonitor.cpp \
			source/SambaMonitor.cpp \
			source/Board.cpp \
			source/MessageEvent.cpp \
			source/OutputWindow.cpp \
			source/OscXmlServer.cpp \
			source/AppUpdater.cpp \
			source/McHelperPrefs.cpp
				
TARGET = mchelper
            
QT += network xml
DEFINES     += MCHELPER_VERSION=\\\"$${MCHELPER_VERSION}\\\"
INCLUDEPATH += include
RESOURCES     += resources/mchelper.qrc
OBJECTS_DIR  = tmp
MOC_DIR      = tmp
DESTDIR      = bin

QTDIR_build:REQUIRES="contains(QT_CONFIG, small-config)"

# ------------------------------------------------------------------
# ------------------------------------------------------------------

macx{
	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.3
	QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk #need this if building on PPC
	
	# Carbon-Cocoa interface for Sparkle
	HEADERS += include/carbon_cocoa.h
	SOURCES += source/carbon_cocoa.mm
	
	#CONFIG += x86 ppc
  LIBS += -framework IOKit -framework Sparkle
  ICON = resources/mchelper.icns
	QMAKE_INFO_PLIST = resources/osx/Info.plist
}


win32{
  LIBS += -lSetupapi
  RC_FILE = resources/mchelper.rc # for application icon
  DEFINES += WINVER=0x0501
  
  # If in debug mode, let's show the output of any
  # q[Debug|Warning|Critical|Fatal]() calls to the console
  #debug {
    #CONFIG += console
  #}
}


unix{

}





