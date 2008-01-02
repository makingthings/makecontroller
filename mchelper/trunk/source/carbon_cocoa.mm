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
	
	Make sure to select the files that you want to call any of these functions in, and set their filetype in Xcode
	to cpp/objcpp so they get processed and mashed together properly.
*/

#include <AppKit/AppKit.h>
#include <Sparkle/SUUpdater.h>

#include "carbon_cocoa.h"

// Initialize Cocoa and Sparkle
void Cocoa::initialize()
{	
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	NSApplicationLoad();	//Start Cocoa interface (requires 10.2 or later)
	SUUpdater* updater = [SUUpdater alloc];
	[updater checkForUpdatesInBackground];
	
	[pool release];
}

void Cocoa::checkForUpdates()
{
	SUUpdater* updater = [SUUpdater alloc];
	[updater checkForUpdates:nil];
}

QString Cocoa::FSRefToPath(FSRef fsref)
{
	CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, &fsref);
	if( !url )
		return QString();
	NSString* path = (NSString*)CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	[path autorelease];
	CFRelease(url);
	return QString::fromUtf8([path UTF8String]);
}



