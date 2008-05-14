#include <QFileDialog>
#include <QSettings>
#include <QTextStream>
#include <QTextEdit>
#include <QDate>
#include <QDesktopServices>
#include <QDomDocument>
#include <QUrl>
#include <QMessageBox>
#include "MainWindow.h"

#define RECENT_FILES 5

/**
	MainWindow represents, not surprisingly, the main window of the application.
	It handles all the menu items and the UI.
*/
MainWindow::MainWindow( ) : QMainWindow( 0 )
{
	setupUi(this);
  //initialization
	prefs = new Preferences(this);
	props = new ProjectProperties(this);
	uploader = new Uploader(this);
	builder = new Builder(this);
  usbMonitor = new UsbMonitor();
  findReplace = new FindReplace(this);
  
	setupEditor( );
	boardTypeGroup = new QActionGroup(menuBoard_Type);
	loadBoardProfiles( );
	loadExamples( );
	loadLibraries( );
	loadRecentProjects( );
  readSettings( );
  
  // misc. signals
	connect(editor, SIGNAL(cursorPositionChanged()), this, SLOT(onCursorMoved()));
	connect(actionPreferences, SIGNAL(triggered()), prefs, SLOT(loadAndShow()));
  connect(actionUsb_Monitor, SIGNAL(triggered()), usbMonitor, SLOT(loadAndShow()));
	connect(currentFileDropDown, SIGNAL(currentIndexChanged(QString)), this, SLOT(onFileSelection(QString)));
  connect(editor->document(), SIGNAL(contentsChanged()),this, SLOT(onDocumentModified()));
  
	// menu actions
	connect(actionNew,					SIGNAL(triggered()), this,		SLOT(onNewFile()));
	connect(actionNew_Project,	SIGNAL(triggered()), this,		SLOT(onNewProject()));
	connect(actionOpen,					SIGNAL(triggered()), this,		SLOT(onOpen()));
	connect(actionSave,					SIGNAL(triggered()), this,		SLOT(onSave()));
	connect(actionSave_As,			SIGNAL(triggered()), this,		SLOT(onSaveAs()));
	connect(actionBuild,				SIGNAL(triggered()), this,		SLOT(onBuild()));
  connect(actionClean,				SIGNAL(triggered()), this,		SLOT(onClean()));
	connect(actionProperties,		SIGNAL(triggered()), this,		SLOT(onProperties()));
	connect(actionUpload,				SIGNAL(triggered()), this,		SLOT(onUpload()));
	connect(actionUndo,					SIGNAL(triggered()), editor,	SLOT(undo()));
	connect(actionRedo,					SIGNAL(triggered()), editor,	SLOT(redo()));
	connect(actionCut,					SIGNAL(triggered()), editor,	SLOT(cut()));
	connect(actionCopy,					SIGNAL(triggered()), editor,	SLOT(copy()));
	connect(actionPaste,				SIGNAL(triggered()), editor,	SLOT(paste()));
	connect(actionSelect_All,		SIGNAL(triggered()), editor,	SLOT(selectAll()));
  connect(actionFind,         SIGNAL(triggered()), findReplace,	SLOT(show()));
	connect(actionClear_Output_Console,		SIGNAL(triggered()), outputConsole,	SLOT(clear()));
	connect(actionUpload_File_to_Board,		SIGNAL(triggered()), this,	SLOT(onUploadFile()));
	connect(actionMake_Controller_Reference, SIGNAL(triggered()), this, SLOT(openMCReference()));
	connect(menuExamples, SIGNAL(triggered(QAction*)), this, SLOT(onExample(QAction*)));
  connect(menuLibraries, SIGNAL(triggered(QAction*)), this, SLOT(onLibrary(QAction*)));
	connect(actionSave_Project_As, SIGNAL(triggered()), this, SLOT(onSaveProjectAs()));
	connect(menuRecent_Projects, SIGNAL(triggered(QAction*)), this, SLOT(openRecentProject(QAction*)));
}

/*
  Restore the app to its state before it was shut down.
*/
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
  
  QString lastProject = settings.value("lastOpenProject").toString();
  if(!lastProject.isEmpty())
    openProject(lastProject);
	settings.endGroup();
}

/*
  Save app settings.
*/
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
  settings.setValue("lastOpenProject", currentProject);
	settings.endGroup();
}

/*
  The app is closing.
*/
void MainWindow::closeEvent( QCloseEvent *qcloseevent )
{
	(void)qcloseevent;
  maybeSave( );
	writeSettings( );
}

/*
  The cursor has moved.
  Highlight the current line, if appropriate.
  Update the line/column display in the status bar.
*/
void MainWindow::onCursorMoved( )
{
	QTextCursor c = editor->textCursor();
  if(c.hasSelection()) // don't highlight the line if text is selected
    return editor->setExtraSelections(QList<QTextEdit::ExtraSelection>());
  QTextEdit::ExtraSelection highlight;
	highlight.cursor = c;
	highlight.format.setProperty(QTextFormat::FullWidthSelection, true);
	highlight.format.setBackground( QColor::fromRgb(245, 245, 245) ); // light gray

	QList<QTextEdit::ExtraSelection> extras;
	extras << highlight;
	editor->setExtraSelections( extras );
  
  statusBar()->showMessage( QString("Line: %1  Column: %2").arg(c.blockNumber()).arg(c.columnNumber()));
}

/*
  The file in the editor has been changed.
  Set the window 'dirty' state to reflect the state of the file.
*/
void MainWindow::onDocumentModified( )
{
  setWindowModified(editor->document()->isModified());
}

/*
  Find text in the file currently open in the editor.
*/
bool MainWindow::findText(QString text, bool ignoreCase, bool forward, bool wholeword)
{
  QTextDocument::FindFlags flags;
  if(!forward)
    flags |= QTextDocument::FindBackward;
  if(!ignoreCase)
    flags |= QTextDocument::FindCaseSensitively;
  if(wholeword)
    flags |= QTextDocument::FindWholeWords;
  
  bool success = false;
  if(!editor->find(text, flags)) // if we didn't find it, try wrapping around
  {
    if(forward)
      editor->moveCursor(QTextCursor::Start);
    else
      editor->moveCursor(QTextCursor::End);
      
    if(editor->find(text, flags)) // now try again
      success = true;
  }
  else
    success = true;
  return success;
}

/*
  Replace all occurrences of the given text with the given replacement.
  ...this is not the best way to do this.
*/
void MainWindow::replaceAll(QString find, QString replace)
{
  bool keepgoing = true;
  do
  {
    if(editor->find(find))
    {
      QTextCursor cursor = editor->textCursor();
      cursor.clearSelection();
      cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
      cursor.insertText(replace);
    }
    else
      keepgoing = false;
  } while(keepgoing);
}

void MainWindow::setupEditor( )
{
	QSettings settings("MakingThings", "mcbuilder");
	setTabWidth( settings.value("Editor/tabWidth", 2).toInt() );
	highlighter = new Highlighter( editor->document() );
}

void MainWindow::setTabWidth( int width )
{
	QFontMetrics fm(editor->currentFont());
	editor->setTabStopWidth( fm.width(" ") * width );
}

QString MainWindow::currentBoardProfile( )
{
  QAction *board = boardTypeGroup->checkedAction( );
  if(board)
    return boardTypes.value(board->text());
  else
    return QString();
}

/*
  Load a source file into the editor.
*/
void MainWindow::editorLoadFile( QFile *file )
{
	Q_ASSERT(!currentProject.isEmpty());
	if(file->open(QIODevice::ReadOnly|QFile::Text))
	{
		currentFile = file->fileName();
		editor->setPlainText(file->readAll());
		file->close();
    editor->document()->setModified(false);
    setWindowModified(false);
	}
}

/*
  Called when the "new file" action is triggered.
  Prompt for a file name and create it within the
  currently open project.
*/
void MainWindow::onNewFile( )
{
	if(currentProject.isEmpty())
	{
		statusBar()->showMessage( "Need to open a project first.  Open or create a new one from the File menu.", 3500 );
		return;
	}
	QString newFilePath = QFileDialog::getSaveFileName(this, tr("Create New File"), 
																			currentProject, tr("C Files (*.c)"));
	if(!newFilePath.isNull()) // user cancelled
		createNewFile(newFilePath);
}

/*
  Create a new source file.
*/
void MainWindow::createNewFile(QString path)
{
	QDir dir(path);
	QString name = dir.dirName() + ".c";
	QFile file(path + ".c");
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

/*
  A new file has been selected.
  Load it into the editor.
*/
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

/*
  Called when the "new project" action is triggered.
  Create a new project directory and project file within it.
*/
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
  
  // grab the templates for a new project
  QDir templatesDir = QDir::current();
  templatesDir.cd("resources/templates");
  QFile templateFile(templatesDir.filePath("properties_template.xml"));
  if( templateFile.open(QIODevice::ReadOnly | QFile::Text) )
  {
    // create the properties file
    QFile newProjFile(newProj.filePath(newProjName + ".xml"));
    if( newProjFile.open(QIODevice::WriteOnly | QFile::Text) )
    {
      newProjFile.write(templateFile.readAll());
      newProjFile.close();
    }
    templateFile.close();
  }
  
  templateFile.setFileName(templatesDir.filePath("project_template.txt"));
	if( templateFile.open(QIODevice::ReadOnly | QFile::Text) )
  {
    // and create the main file
    QFile mainFile(newProj.filePath(newProjName + ".c"));
    if( mainFile.open(QIODevice::WriteOnly | QFile::Text) )
    {
      QTextStream out(&mainFile);
      out << QString("// %1.c").arg(newProjName) << endl;
      out << QString("// created %1").arg(QDate::currentDate().toString("MMM d, yyyy") ) << endl;
      out << templateFile.readAll();
      mainFile.close();
    }
    templateFile.close();
  }
	openProject(newProjPath);
}

/*
  Called when the "open" action is triggered.
  Prompt the user for which project they'd like to open.
*/
void MainWindow::onOpen( )
{
	QString projectPath = QFileDialog::getExistingDirectory(this, tr("Open Project"), 
																					Preferences::workspace(), QFileDialog::ShowDirsOnly);
	if( !projectPath.isNull() ) // user cancelled
		openProject(projectPath);
}

/*
  Open an existing project.
*/
void MainWindow::openProject(QString projectPath)
{  
  QDir projectDir(projectPath);
	QString projectName = projectDir.dirName();
	if(!projectDir.exists())
		return statusBar()->showMessage( QString("Couldn't find %1.").arg(projectName), 3500 );
	QString mainFileName(projectName + ".c");
	mainFileName.remove(" ");
	QFile mainFile(projectDir.filePath(mainFileName));
	if(mainFile.exists())
	{
		currentProject = projectPath;
		editorLoadFile(&mainFile);
		QStringList projectFiles = projectDir.entryList(QStringList("*.c"));
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

/*
  A new project has been opened.
  If this project isn't already in our recent projects list, stick it in there.
*/
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

/*
  Called when the "save" action is triggered.
*/
void MainWindow::onSave( )
{
	if(currentFile.isEmpty())
		return statusBar()->showMessage( "Need to open a project first.  Open or create a new one from the File menu.", 3500 );
	save( );
}

/*
  Save the file currently in the editor.
*/
bool MainWindow::save( )
{
  QFile file(currentFile);
	if(file.open(QFile::WriteOnly | QFile::Text))
	{
    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    out << editor->toPlainText();
    QApplication::restoreOverrideCursor();
    editor->document()->setModified(false);
    setWindowModified(false);
    return true;
	}
  else
  {
    statusBar()->showMessage( "Couldn't save...maybe the current file has been moved or deleted.", 3500 );
    return false;
  }
}

/*
  Prompt the user to save the current file, if necessary.
  Done before building, or closing, etc.
*/
bool MainWindow::maybeSave()
{
  if (editor->document()->isModified())
  {
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("mcbuilder"),
                               tr("This document has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
      return save();
    else if (ret == QMessageBox::Cancel)
      return false;
  }
  return true;
}

/*
  Called when the "save as" action has been triggered.
  Create a new file with the contents of the current one, 
  rename it and load it into the editor.
*/
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

/*
  Called when the build action is triggered.
  Check to see if we need to save the current file,
  then try to fire off the build process.
*/
void MainWindow::onBuild( )
{
  if(currentProject.isEmpty())
		return statusBar()->showMessage( "Open a project to build, or create a new one from the File menu.", 3500 );
  maybeSave( );
	if(builder->state() == QProcess::NotRunning)
  {
		outputConsole->clear();
    builder->build(currentProject);
  }
	else
		return statusBar()->showMessage( "Builder is currently busy...give it a second, then try again.", 3500 );
}

void MainWindow::onBuildComplete(bool success)
{
  if(success)
    statusBar()->showMessage("Build succeeded.");
  else
    statusBar()->showMessage("Build failed.");
}

void MainWindow::onCleanComplete()
{
  statusBar()->showMessage("Clean succeeded.");
}

void MainWindow::buildingNow(QString file)
{
  statusBar()->showMessage("Building..." + file);
}

/*
  Called when the "clean" action is triggered.
  Fire off the clean process in the builder.
*/
void MainWindow::onClean( )
{
  if(currentProject.isEmpty())
    return;
  if(builder->state() == QProcess::NotRunning)
    builder->clean(currentProject);
	else
		return statusBar()->showMessage( "Builder is currently busy...give it a second, then try again.", 3500 );
}

/*
  Called when the "project properties" action is triggered.
  Pop up the properties dialog.
*/
void MainWindow::onProperties( )
{
	if(currentProject.isEmpty())
		return statusBar()->showMessage( "Open a project first, or create a new one from the File menu.", 3500 );
	if( !props->loadAndShow() )
	{
		QDir dir(currentProject);
		return statusBar()->showMessage( "Couldn't find/open project properties for " + dir.dirName(), 3500 );
	}
}

/*
  Called when the "upload" action is triggered.
  Build the current project and upload the resulting .bin to the board.
*/
void MainWindow::onUpload( )
{
	if(currentProject.isEmpty())
		return statusBar()->showMessage( "Open a project to upload, or create a new one from the File menu.", 3500 );
	uploadFile("temp.bin");
}

/*
  Called when the "upload file" action is triggered.
  Pop up a file dialog, and try to upload the given file
  to the board.  This is only for uploading pre-built .bins.
*/
void MainWindow::onUploadFile( )
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                 QDir::homePath(), tr("Binaries (*.bin)"));
	if(!fileName.isNull())
		uploadFile(fileName);
}

/*
  Actually upload the file to the board.
  Pass in the board config file name to upload a project to that kind of board
*/
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
	dir.cd("resources/board_profiles");
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
					if(boardName == Preferences::boardType( ))
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

/*
  Navigate the examples directory and create menu items
  for each of the available examples.
*/
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

/*
  Called when an action in the Examples menu is triggered.
  Find the example's project and try to open it.
*/
void MainWindow::onExample(QAction *example)
{
	QMenu *menu = (QMenu*)example->parent();
	QString examplePath = QDir::currentPath();
	examplePath += "/examples/" + menu->title() + "/" + example->text();
	openProject(examplePath);
}

/*
  Navigate the libraries directory and load the available ones into the UI.
  Also add library examples to the examples menu, and example
  keywords to the syntax highlighter's keyword list.
*/
void MainWindow::loadLibraries( )
{
  QDir dir = QDir::current();
	dir.cd("libraries");
	QStringList libraries = dir.entryList(QStringList(), QDir::Dirs | QDir::NoDotAndDotDot);
	foreach(QString library, libraries)
	{
    // add the library to the "Import Library" menu
    QAction *a = new QAction(library, menuLibraries);
    menuLibraries->addAction(a);
    // add the library's keywords to the syntax highlighter
    // add the library's examples to the example menu
	}
}

/*
  Called when an item in the Libraries menu is triggered.
  Add a #include into the current document for the selected library
*/
void MainWindow::onLibrary(QAction *example)
{
  QString includeString = QString("#include %1.h").arg(example->text());
  // only add if it isn't already in there
  // find() moves the cursor and highlights the found text
  if(!editor->find(includeString) && !editor->find(includeString, QTextDocument::FindBackward))
  {
    editor->moveCursor(QTextCursor::Start);
    editor->append(includeString);
  }
}

/*
  Populate the recent projects menu.
*/
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
	outputConsole->moveCursor(QTextCursor::End);
  outputConsole->insertPlainText(text);
}

void MainWindow::printOutputError(QString text)
{
	outputConsole->moveCursor(QTextCursor::End);
  outputConsole->insertPlainText(text);
}

void MainWindow::openMCReference( )
{
	QDir dir = QDir::current();
	dir.cd("reference/makecontroller");
	QDesktopServices::openUrl(QUrl::fromLocalFile(dir.filePath("index.html")));
}




