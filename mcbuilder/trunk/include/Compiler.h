#ifndef COMPILER_H
#define COMPILER_H

#include <QProcess>
#include "Compiler.h"

class MainWindow;

class Compiler : public QObject
{
	Q_OBJECT
	public:
		Compiler(MainWindow *mainWindow);
		void compile(QString projectName);
	private:
		MainWindow *mainWindow;
		QProcess *compiler;

};

#endif // COMPILER_H