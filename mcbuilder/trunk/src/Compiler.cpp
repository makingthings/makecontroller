
#include "Compiler.h"

Compiler::Compiler(MainWindow *mainWindow) : QObject( 0 )
{
	this->mainWindow = mainWindow;
	compiler = new QProcess(this);
}

void Compiler::compile(QString projectName)
{
	projectName = "";
}

