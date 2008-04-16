#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include "ui_preferences.h"
#include "MainWindow.h"

class MainWindow;

class Preferences : public QDialog, private Ui::Preferences
{
	Q_OBJECT
	public:
		Preferences(MainWindow *mainWindow);
		static QString workspace( );
		static QString boardType( );
		
	private:
		MainWindow *mainWindow;
		
	public slots:
		void loadAndShow( );
	private slots:
		void applyChanges( );
		void browseWorkspace( );
};

#endif // PREFERENCES_H