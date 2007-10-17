/*
 * taken from the Axel project (http://excalibur.inria.fr/),
 * licensed under GPL
 */
#ifndef COCOAUTIL_H
#define COCOAUTIL_H

#include <QString>
#include <Carbon/Carbon.h>

class SUUpdater;

namespace CocoaUtil 
{
	void initialize();
	void checkForUpdates();
	QString FSRefToPath(FSRef ref);
};

#endif

