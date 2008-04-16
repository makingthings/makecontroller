#ifndef PROJECT_PROPERTIES_H
#define PROJECT_PROPERTIES_H

#include <QDialog>
#include <QDomDocument>
#include "MainWindow.h"
#include "ui_properties.h"

class MainWindow;

class ProjectProperties : public QDialog, private Ui::Properties
{
	Q_OBJECT
	public:
		ProjectProperties(MainWindow *mainWindow);
	public slots:
		bool loadAndShow();
	private:
		MainWindow *mainWindow;
		QDomDocument propsFile;
		QString propFilePath( );
	private slots:
		void applyChanges( );

};

#endif // PROJECT_PROPERTIES_H