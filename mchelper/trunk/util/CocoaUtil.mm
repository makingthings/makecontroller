/*
 * taken from the Axel project (http://excalibur.inria.fr/),
 * licensed under GPL
 */
#include <AppKit/AppKit.h>
#include <Sparkle/SUUpdater.h>

#include "CocoaUtil.h"

void CocoaUtil::initialize()
{
	NSApplicationLoad();
	SUUpdater * updater = [SUUpdater alloc];
	[updater checkForUpdatesInBackground];
}

void CocoaUtil::checkForUpdates()
{
	SUUpdater * updater = [SUUpdater alloc];
	[updater checkForUpdates:nil];
}

QString CocoaUtil::FSRefToPath(FSRef fsref)
{
    CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, &fsref);
    if (!url)
    {
        return QString();
    }
    NSString * pathName = (NSString*)CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
    [pathName autorelease];
    CFRelease(url);
    return QString::fromUtf8([pathName UTF8String]);
}
