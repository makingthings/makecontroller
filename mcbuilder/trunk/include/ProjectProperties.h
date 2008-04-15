#ifndef PROJECT_PROPERTIES_H
#define PROJECT_PROPERTIES_H

#include <QDialog>
#include "MainWindow.h"

class MainWindow;

class ProjectProperties : public QDialog
{
	Q_OBJECT
	public:
		ProjectProperties(MainWindow *mainWindow);
	private:
		MainWindow *mainWindow;

};

#endif // PROJECT_PROPERTIES_H