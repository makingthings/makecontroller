#include <QFileDialog>
#include <QSettings>
#include "Preferences.h"

Preferences::Preferences(MainWindow *mainWindow) : QDialog( 0 )
{
	this->mainWindow = mainWindow;
	setupUi(this);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(applyChanges()));
	connect(browseWorkspaceButton, SIGNAL(clicked()), this, SLOT(browseWorkspace()));
}

// static
QString Preferences::workspace( )
{
	QSettings settings("MakingThings", "mcbuilder");
	#ifdef Q_WS_MAC
	QString workspace = QDir::home().path() + QDir::toNativeSeparators("/Documents/mcbuilder");
	#else
	QString workspace = QDir::home().path() + "mcbuilder";
	#endif
	workspace = settings.value("General/workspace", workspace).toString();
	// always make sure the workspace directory exists
	QDir dir(workspace);
	if(!dir.exists())
	{
		dir.cdUp();
		dir.mkdir("mcbuilder");
	}
	return workspace;
}

// static
QString Preferences::boardType( )
{
	QSettings settings("MakingThings", "mcbuilder");
	// select Make Controller by default
	return settings.value("General/boardType", "Make Controller").toString();
}

// read the current settings, load them into the preferences form and then display it
void Preferences::loadAndShow( )
{
	QSettings settings("MakingThings", "mcbuilder");
	settings.beginGroup("General");
	workspaceEdit->setText( Preferences::workspace() );
	settings.endGroup();
	
	settings.beginGroup("Editor");
	tabWidth->setText(QString::number(settings.value("tabWidth", 2).toInt()));
	settings.endGroup();
	// finally
	this->show( );
}

// when the "browse" button is clicked in the general prefs, this handles it
void Preferences::browseWorkspace( )
{
	QString dummy;
	QString newProjDir = QFileDialog::getOpenFileName(this, tr("Select Project Directory"), 
																					Preferences::workspace(), "", &dummy, QFileDialog::ShowDirsOnly);
	if( !newProjDir.isNull() ) // will be null if user hit cancel
		workspaceEdit->setText(newProjDir);
}

// rip through the preferences items, see if any have changed 
// and call the mainwindow back if it needs to be updated
void Preferences::applyChanges( )
{
	QSettings settings("MakingThings", "mcbuilder");
	
	settings.beginGroup("General");
	settings.setValue("workspace", workspaceEdit->text());
	settings.endGroup();
	
	settings.beginGroup("Editor");
	int oldTabWidth = settings.value("tabWidth").toInt();
	if( oldTabWidth != tabWidth->text().toInt() )
	{
		settings.setValue("tabWidth", tabWidth->text().toInt());
		mainWindow->setTabWidth( tabWidth->text().toInt() );
	}
	settings.endGroup();
}




