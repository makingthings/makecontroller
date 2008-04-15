#include <QFileDialog>
#include <QSettings>
#include <QTextStream>
#include <QTextEdit>
#include <QDate>
#include <QDesktopServices>
#include <QDomDocument>
#include <QUrl>
#include "MainWindow.h"

#define RECENT_FILES 5

/**
	MainWindow represents, not surprisingly, the main window of the application.
	It handles all the menu items and the UI.
*/
MainWindow::MainWindow( ) : QMainWindow( 0 )
{
	setupUi(this);
	readSettings( );
	splitter->setChildrenCollapsible( false );
	prefs = new Preferences( this );
	uploader = new Uploader(this);
	setupEditor( );
	boardTypeGroup = new QActionGroup(menuBoard_Type);
	loadBoardProfiles( );
	loadExamples( );
	loadLibraries( );
	loadRecentProjects( );
	connect(editor, SIGNAL(cursorPositionChanged()), this, SLOT(highlightLine()));
	//connect(editor, SIGNAL(undoAvailable(bool)), this, SLOT(setWindowModified(bool)));
	connect(actionPreferences, SIGNAL(triggered()), prefs, SLOT(loadAndShow()));
	connect(currentFileDropDown, SIGNAL(currentIndexChanged(QString)), this, SLOT(onFileSelection(QString)));
	// menu actions
	connect(actionNew,					SIGNAL(triggered()), this,		SLOT(onNewFile()));
	connect(actionNew_Project,	SIGNAL(triggered()), this,		SLOT(onNewProject()));
	connect(actionOpen,					SIGNAL(triggered()), this,		SLOT(onOpen()));
	connect(actionSave,					SIGNAL(triggered()), this,		SLOT(onSave()));
	connect(actionSave_As,			SIGNAL(triggered()), this,		SLOT(onSaveAs()));
	connect(actionBuild,				SIGNAL(triggered()), this,		SLOT(onBuild()));
	connect(actionProperties,		SIGNAL(triggered()), this,		SLOT(onProperties()));
	connect(actionUpload,				SIGNAL(triggered()), this,		SLOT(onUpload()));
	connect(actionUndo,					SIGNAL(triggered()), editor,	SLOT(undo()));
	connect(actionRedo,					SIGNAL(triggered()), editor,	SLOT(redo()));
	connect(actionCut,					SIGNAL(triggered()), editor,	SLOT(cut()));
	connect(actionCopy,					SIGNAL(triggered()), editor,	SLOT(copy()));
	connect(actionPaste,				SIGNAL(triggered()), editor,	SLOT(paste()));
	connect(actionSelect_All,		SIGNAL(triggered()), editor,	SLOT(selectAll()));
	connect(actionClear_Output_Console,		SIGNAL(triggered()), outputConsole,	SLOT(clear()));
	connect(actionUpload_File_to_Board,		SIGNAL(triggered()), this,	SLOT(onUploadFile()));
	connect(actionMake_Controller_Reference, SIGNAL(triggered()), this, SLOT(openMCReference()));
	connect(menuExamples, SIGNAL(triggered(QAction*)), this, SLOT(onExample(QAction*)));
	connect(actionSave_Project_As, SIGNAL(triggered()), this, SLOT(onSaveProjectAs()));
	connect(menuRecent_Projects, SIGNAL(triggered(QAction*)), this, SLOT(openRecentProject(QAction*)));
}

void MainWindow::readSettings()
{
	QSettings settings("MakingThings", "mcbuilder");
	settings.beginGroup("MainWindow");
	
	QSize size = settings.value( "size" ).toSize( );
	if( size.isValid( ) )
		resize( size );
	
	QList<QVariant> splitterSettings = settings.value( "splitterSizes" ).toList( );
	QList<int> splitterSizes;
	if( !splitterSettings.isEmpty( ) )
	{
		for( int i = 0; i < splitterSettings.count( ); i++ )
			splitterSizes.append( splitterSettings.at(i).toInt( ) );
		splitter->setSizes( splitterSizes );
	}
	settings.endGroup();
}

void MainWindow::writeSettings()
{
	QSettings settings("MakingThings", "mcbuilder");
	settings.beginGroup("MainWindow");
	settings.setValue("size", size() );
	QList<QVariant> splitterSettings;
	QList<int> splitterSizes = splitter->sizes();
	for( int i = 0; i < splitterSizes.count( ); i++ )
		splitterSettings.append( splitterSizes.at(i) );
	settings.setValue("splitterSizes", splitterSettings );
	settings.endGroup();
}

void MainWindow::closeEvent( QCloseEvent *qcloseevent )
{
	(void)qcloseevent;
	writeSettings( );
}

// when the cursor is moved, this is called to highlight the current line
void MainWindow::highlightLine( )
{
	QTextEdit::ExtraSelection highlight;
	highlight.cursor = editor->textCursor();
	highlight.format.setProperty(QTextFormat::FullWidthSelection, true);
	highlight.format.setBackground( Qt::green );

	QList<QTextEdit::ExtraSelection> extras;
	extras << highlight;
	editor->setExtraSelections( extras );
}

void MainWindow::setupEditor( )
{
	QSettings settings("MakingThings", "mcbuilder");
	setTabWidth( settings.value("editor/tabWidth", 2).toInt() );
	highlighter = new Highlighter( editor->document() );
}

void MainWindow::setTabWidth( int width )
{
	QFontMetrics fm(editor->currentFont());
	editor->setTabStopWidth( fm.width(" ") * width );
}

void MainWindow::editorLoadFile( QFile *file )
{
	Q_ASSERT(!currentProject.isEmpty());
	if( file->open(QIODevice::ReadOnly) )
	{
		currentFile = file->fileName();
		editor->setPlainText(file->readAll());
		file->close();
	}
}

// create a new file...within the existing project if one is open
// otherwise create a new project
void MainWindow::onNewFile( )
{
	if(currentProject.isEmpty())
	{
		statusBar()->showMessage( "Need to open a project first.  Open or create a new one from the File menu.", 3500 );
		return;
	}
	QString newFilePath = QFileDialog::getSaveFileName(this, tr("Create New File"), 
																			currentProject, tr("CPP Files (*.cpp)"));
	if(!newFilePath.isNull()) // user cancelled
		createNewFile(newFilePath);
}

void MainWindow::createNewFile(QString path)
{
	QDir dir(path);
	QString name = dir.dirName() + ".cpp";
	QFile file(path + ".cpp");
	if(file.exists())
		return;
	if(file.open(QIODevice::WriteOnly | QFile::Text))
	{
		QTextStream out(&file);
		out << QString("// %1").arg(name) << endl;
		out << QString("// created %1").arg(QDate::currentDate().toString("MMM d, yyyy") ) << endl << endl;
		file.close();
		editorLoadFile(&file);
		currentFileDropDown->addItem(name);
		currentFileDropDown->setCurrentIndex(currentFileDropDown->findText(name));
	}
}

// a new file has been selected 
void MainWindow::onFileSelection(QString filename)
{
	Q_ASSERT(!currentProject.isEmpty()); // we shouldn't have any files loaded unless a project is open
	QDir dir(currentProject);
	QFile file(dir.filePath(filename));
	if(file.exists())
		editorLoadFile(&file);
	else
	{
		// TODO: remove this from the file dropdown...
		QString message = QString("Couldn't find %1 in %2.").arg(filename).arg(dir.dirName());
		//currentFileDropDown->removeItem(currentFileDropDown->findText(name));
		statusBar()->showMessage(message, 3000);
	}
	
}

// create a new project directory and project file within it
void MainWindow::onNewProject( )
{	
	QString dummy;
	QString workspace = Preferences::workspace();
	QString newProjPath = QFileDialog::getSaveFileName(this, tr("Create Project"), 
																		workspace, "", &dummy, QFileDialog::ShowDirsOnly);
	if( newProjPath.isNull() )
		return;
	// create a directory for the project
	QDir workspaceDir(workspace);
	workspaceDir.mkdir(newProjPath);
	QDir newProj(newProjPath);
	QString newProjName = newProj.dirName().remove(" "); // file names shouldn't have any spaces
		
	// create the project file
	QFile newProjFile(newProj.filePath(newProjName + ".mcbld"));
	if( newProjFile.open(QIODevice::WriteOnly | QFile::Text) )
	{
		newProjFile.write(QString("").toUtf8());
		newProjFile.close();
	}
	
	// and create the main file
	QFile mainFile(newProj.filePath(newProjName + ".cpp"));
	if( mainFile.open(QIODevice::WriteOnly | QFile::Text) )
	{
		QTextStream out(&mainFile);
		out << QString("// %1.cpp").arg(newProjName) << endl;
		out << QString("// created %1").arg(QDate::currentDate().toString("MMM d, yyyy") ) << endl << endl;
		out << "setup( )" << endl << "{" << endl << endl << "}" << endl << endl;
		out << "loop( )" << endl << "{" << endl << endl << "}" << endl;
		mainFile.close();
	}
	openProject(newProjPath);
}

void MainWindow::onOpen( )
{
	QString projectPath = QFileDialog::getExistingDirectory(this, tr("Open Project"), 
																					Preferences::workspace(), QFileDialog::ShowDirsOnly);
	if( !projectPath.isNull() ) // user cancelled
		openProject(projectPath);
}

void MainWindow::openProject(QString projectPath)
{
	QDir projectDir(projectPath);
	QString projectName = projectDir.dirName();
	if(!projectDir.exists())
		return statusBar()->showMessage( QString("Couldn't find %1.").arg(projectName), 3500 );
	QString mainFileName(projectName + ".cpp");
	mainFileName.remove(" ");
	QFile mainFile(projectDir.filePath(mainFileName));
	if(mainFile.exists())
	{
		currentProject = projectPath;
		editorLoadFile(&mainFile);
		QStringList projectFiles = projectDir.entryList(QStringList("*.cpp"));
		// update the files in the dropdown list
		currentFileDropDown->clear();
		currentFileDropDown->insertItems(0, projectFiles);
		currentFileDropDown->setCurrentIndex(currentFileDropDown->findText(mainFileName));
		setWindowTitle( projectName + "[*] - mcbuilder");
		updateRecentProjects(projectName);			
	}
	else
		return statusBar()->showMessage( QString("Couldn't find main file for %1.").arg(projectName), 3500 );
}

void MainWindow::updateRecentProjects(QString newProject)
{
	QList<QAction*> recentActions = menuRecent_Projects->actions();
	QStringList projects;
	foreach(QAction* a, recentActions)
		projects << a->text();
	if(!projects.contains(newProject)) // only need to update if this project is not already included
	{
		if(recentActions.count() >= RECENT_FILES )
			menuRecent_Projects->removeAction(recentActions.at(0));
		menuRecent_Projects->addAction(newProject);
		foreach(QAction *a, menuRecent_Projects->actions()) // do this again to update our stringlist since it has changed
			projects << a->text();
		QSettings settings("MakingThings", "mcbuilder");
		settings.setValue("MainWindow/recentProjects", projects);
	}	
}

void MainWindow::openRecentProject(QAction* project)
{
	QDir dir(Preferences::workspace());
	openProject(dir.filePath(project->text( )));
}

void MainWindow::onSave( )
{
	if(currentFile.isEmpty())
		return statusBar()->showMessage( "Need to open a project first.  Open or create a new one from the File menu.", 3500 );
	QFile file(currentFile);
	if (file.open(QFile::WriteOnly | QFile::Text))
	{
		file.write(editor->toPlainText().toUtf8());
		file.close();
	}
}

void MainWindow::onSaveAs( )
{
	if(currentFile.isEmpty())
		return statusBar()->showMessage( "Need to open a project first.  Open or create a new one from the File menu.", 3500 );
		
	QString newFileName = QFileDialog::getSaveFileName(this, tr("Save As"), 
																			currentProject, tr("CPP Files (*.cpp)"));
	if( newFileName.isNull() ) // user cancelled
		return;
		
	QFile file(currentFile);
	if(!newFileName.endsWith(".cpp"))
		newFileName.append(".cpp");
	file.copy(newFileName);
	QFile newFile(newFileName);
	editorLoadFile(&newFile);
	QFileInfo fi(newFile);
	currentFileDropDown->addItem(fi.fileName());
	currentFileDropDown->setCurrentIndex(currentFileDropDown->findText(fi.fileName()));
}

void MainWindow::onSaveProjectAs( )
{
	
}

void MainWindow::onBuild( )
{
	
}

void MainWindow::onProperties( )
{
	
}

void MainWindow::onUpload( )
{
	uploadFile("temp.bin");
}

void MainWindow::onUploadFile( )
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                 QDir::homePath(), tr("Binaries (*.bin)"));
	if(!fileName.isNull())
		uploadFile(fileName);
}

// pass in the board config file name to upload a project to that kind of board
void MainWindow::uploadFile(QString filename)
{
	// get the board type so the uploader can check which upload mechanism to use
	QAction *board = boardTypeGroup->checkedAction( );
	if(board)
	{
		if(uploader->state() == QProcess::NotRunning)
			uploader->upload(boardTypes.value(board->text()), filename);
		else
			return statusBar()->showMessage( "Uploader is currently busy...give it a second, then try again.", 3500 );
	}
	else
		return statusBar()->showMessage( "Please select a board type from the Project menu first.", 3500 );
}

// read the available board files and load them into the UI
void MainWindow::loadBoardProfiles( )
{
	QDir dir = QDir::current();
	dir.cd("boards");
	QStringList boardProfiles = dir.entryList(QStringList("*.xml"));
	QDomDocument doc;
	// get a list of the names of the actions we already have
	QList<QAction*> boardActions = menuBoard_Type->actions( );
	QStringList actionNames;
	foreach(QAction *a, boardActions)
		actionNames << a->text();
	
	foreach(QString filename, boardProfiles)
	{
		QFile file(dir.filePath(filename));
		if(file.open(QIODevice::ReadOnly))
		{
			if(doc.setContent(&file))
			{
				QString boardName = doc.elementsByTagName("name").at(0).toElement().text();
				if(!actionNames.contains(boardName))
				{
					QAction *boardAction = new QAction(boardName, this);
					boardAction->setCheckable(true);
					if(boardName == "Make Controller") // select Make Controller by default
						boardAction->setChecked(true);
					menuBoard_Type->addAction(boardAction); // add the action to the actual menu
					boardTypeGroup->addAction(boardAction); // and to the group that maintains an exclusive selection within the menu
					boardTypes.insert(boardName, filename); // hang onto the filename so we don't have to look it up again later
				}
			}
			file.close();
		}
	}
}

void MainWindow::loadExamples( )
{
	QDir dir = QDir::current();
	dir.cd("examples");
	QStringList exampleCategories = dir.entryList(QStringList(), QDir::Dirs | QDir::NoDotAndDotDot);
	foreach(QString category, exampleCategories)
	{
		QMenu *exampleMenu = new QMenu(category, this);
		menuExamples->addMenu(exampleMenu);
		QDir exampleDir(dir.path());
		exampleDir.cd(category);
		QStringList examples = exampleDir.entryList(QStringList(), QDir::Dirs | QDir::NoDotAndDotDot);
		foreach(QString example, examples)
		{
			QAction *a = new QAction(example, exampleMenu);
			exampleMenu->addAction(a);
		}
	}
}

void MainWindow::onExample(QAction *example)
{
	QMenu *menu = (QMenu*)example->parent();
	QString examplePath = QDir::currentPath();
	QString s = QDir::separator();
	examplePath += s + "examples" + s + menu->title() + s + example->text();
	openProject(examplePath);
}

void MainWindow::loadLibraries( )
{

}

void MainWindow::loadRecentProjects( )
{
	QSettings settings("MakingThings", "mcbuilder");
	QStringList projects = settings.value("MainWindow/recentProjects").toStringList();
	projects = projects.mid(0,RECENT_FILES); // just in case there are extras
	foreach(QString project, projects)
		menuRecent_Projects->addAction(project);
}

void MainWindow::printOutput(QString text)
{
	outputConsole->insertPlainText(text);
	outputConsole->ensureCursorVisible();
}

void MainWindow::printOutputError(QString text)
{
	outputConsole->insertPlainText(text);
	outputConsole->ensureCursorVisible();
}

void MainWindow::openMCReference( )
{
	QDir dir = QDir::current();
	dir.cd("ref/makecontroller");
	QDesktopServices::openUrl(QUrl::fromLocalFile(dir.filePath("index.html")));
}




