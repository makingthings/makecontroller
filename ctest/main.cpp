/****************************************************************************
**
** Qt SAM7X Uploader.
** MakingThings 2006.
**
****************************************************************************/

#include "CTestWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	CTestWindow cTestWindow( &app );
	
	cTestWindow.show();
	return app.exec();
}
