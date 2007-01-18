# ------------------------------------------------------------------------------
#
# Copyright 2006 MakingThings
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


TEMPLATE	= app

FORMS = mchelper.ui

CONFIG		+= qt warn_on debug

HEADERS		= McHelperWindow.h \
				UploaderThread.h \
				PacketInterface.h \
				PacketReadyInterface.h \
				PacketUdp.h \
				Osc.h \
				Samba.h \
				UsbSerial.h \
				PacketUsbCdc.h
            
SOURCES		= main.cpp \
				McHelperWindow.cpp \
				UploaderThread.cpp \
				PacketUdp.cpp \
				Osc.cpp \
				Samba.cpp \
				UsbSerial.cpp \
				PacketUsbCdc.cpp 
				
TARGET		= mchelper
            
QT += network

QTDIR_build:REQUIRES="contains(QT_CONFIG, small-config)"

# ------------------------------------------------------------------
# ------------------------------------------------------------------

macx{
  message("This project is being built on a Mac.")
  LIBS += -framework IOKit
  LIBS+= -dead_strip
  ICON = IconPackageOSX.icns
  QMAKE_POST_LINK = strip mchelper.app/Contents/MacOS/mchelper
}


win32{
  message("This project is being built on Windows.")
  DEFINES += __LITTLE_ENDIAN__
  LIBS += -lSetupapi
  RC_FILE = mchelper.rc # for application icon
}


unix{
  !macx{
    message("This project is being built on Linux.")
    DEFINES += Q_WS_LINUX __LITTLE_ENDIAN__
  }
}





