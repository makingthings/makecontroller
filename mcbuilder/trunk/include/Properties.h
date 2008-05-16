#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <QDialog>
#include <QDomDocument>
#include "MainWindow.h"
#include "ui_properties.h"

class MainWindow;

class Properties : public QDialog, private Ui::Properties
{
	Q_OBJECT
	public:
		Properties(MainWindow *mainWindow);
    QString optLevel();
    bool debug();
	public slots:
		bool loadAndShow();
	private:
		MainWindow *mainWindow;
		QDomDocument propsFile;
		QString propFilePath( );
	private slots:
		void applyChanges( );

};

#endif // PROPERTIES_H