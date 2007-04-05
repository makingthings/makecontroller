/****************************************************************************
**
** Qt SAM7X Uploader.
** MakingThings 2006.
**
****************************************************************************/

#include "ATestWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	ATestWindow aTestWindow( &app );
	
	aTestWindow.show();
	return app.exec();
}
