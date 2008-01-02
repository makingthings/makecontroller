/*********************************************************************************

 Copyright 2006-2007 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

/*
	Wrappers for calling Cocoa from Carbon (because Qt is Carbonized)
	Specialized for handling Sparkle interface

	Apple's "Carbon-Cocoa Integration Guide" explains the basics:
	http://developer.apple.com/documentation/Cocoa/Conceptual/CarbonCocoaDoc/index.html#//apple_ref/doc/uid/TP30000893

	Created June 30, 2007	Brandon Fosdick <bfoz@bfoz.net>
*/

#ifndef CARBON_COCOAH
#define	CARBON_COCOAH

#include <Carbon/Carbon.h>
#include <QString>

class SUUpdater;

namespace Cocoa
{
	void initialize();
	void checkForUpdates();
	QString FSRefToPath(FSRef);
};

#endif	// CARBON_COCOAH