/*********************************************************************************

 Copyright 2008-2009 MakingThings

 Licensed under the Apache License,
 Version 2.0 (the "License"); you may not use this file except in compliance
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/


#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include "Builder.h"

#define CORES_DIR QString("cores/")
#define LIBRARIES_DIR (CORES_DIR + "makecontroller/libraries")

/*
  Builder takes a project and turns it into a binary executable.
  We need to generate a Makefile based on the general Preferences
  and Properties for this project.
*/
Builder::Builder(MainWindow *mainWindow, ProjectInfo *projInfo, BuildLog *buildLog, Preferences* prefs) : QProcess(0)
{
  this->mainWindow = mainWindow;
  this->projInfo = projInfo;
  this->buildLog = buildLog;
  this->prefs = prefs;

  connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(filterOutput()));
  connect(this, SIGNAL(readyReadStandardError()), this, SLOT(filterErrorOutput()));
  connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(nextStep(int, QProcess::ExitStatus)));
  connect(this, SIGNAL(error(QProcess::ProcessError)), this, SLOT(onBuildError(QProcess::ProcessError)));
}

/*
  Prepare the build process for the given project, then fire it off.
*/
void Builder::build(const QString & projectName)
{
  currentProjectPath = projectName;
  QDir dir(projectName);
  setWorkingDirectory(projectName);

  buildStep = BUILD;
  currentProcess = "make";
  setEnvironment(QProcess::systemEnvironment());
  QString makePath = Preferences::makePath();
  if (!makePath.isEmpty() && !makePath.endsWith("/"))  // if this is empty, just leave it so the system versions are used
    makePath += "/";
  start(makePath + "make", generateArgs(projectName));
  QString buildmsg("***************************************************************\n");
  buildmsg += tr("  mcbuilder - building ") + dir.dirName() + "\n";
  buildmsg += QDateTime::currentDateTime().toString("  MMM d, yyyy h:m ap") + "\n";
  buildmsg += "***************************************************************";
  buildLog->append(buildmsg);
}

QStringList Builder::generateArgs(const QString & projectName)
{
  QStringList args;
  int parallelthreads = QThread::idealThreadCount();
  if (parallelthreads > 1)
    args << QString("-j%1").arg(parallelthreads); // use all the cores possible

  QString coresDir = CORES_DIR + "makecontroller";
  #ifdef MCBUILDER_TEST_SUITE
    coresDir.prepend("../");
  #endif
  QString src = QDir::cleanPath(MainWindow::appDirectory().filePath(coresDir));
  args << QString("PROJECT=%1").arg(projectName.split("/").last());
  args << QString("LWIP=%1/core/lwip").arg(src);
  args << QString("USB=%1/core").arg(src);
  args << QString("CHIBIOS=%1/core/chibios").arg(src);
  args << QString("MT=%1/core/makingthings").arg(src);
  args << QString("LIBRARIES=%1/libraries").arg(src);
  args << QString("TRGT=%1/arm-none-eabi-").arg(Preferences::toolsPath());

  // load up the list of libraries this project depends on
  QString libsPath = MainWindow::appDirectory().filePath(LIBRARIES_DIR);
  QList<Builder::Library> libs = loadDependencies(libsPath, projectName);
  QString csrc = "MCBUILDER_CSRC=";
  QString cppsrc = "MCBUILDER_CPPSRC=";
  QString incdir = "MCBUILDER_INCDIR=";
  foreach (Builder::Library lib, libs) {
    foreach (QString cfile, lib.csrc)
      csrc.append(cfile + "\\ ");
    foreach (QString cppfile, lib.cppsrc)
      csrc.append(cppsrc + "\\ ");
    incdir.append(QDir(LIBRARIES_DIR).filePath(lib.name));
  }
  // if we actually got anything in any of them, add them to the list of args
  if (!csrc.endsWith("=")) args << csrc;
  if (!cppsrc.endsWith("=")) args << cppsrc;
  if (!incdir.endsWith("=")) args << incdir;
  return args;
}

/*
  Remove all the object files from the build directory.
*/
void Builder::clean(const QString & projectName)
{
  setWorkingDirectory(projectName);
  buildStep = CLEAN;
  currentProcess = "make clean";
  buildLog->clear();
  setEnvironment(QProcess::systemEnvironment());
  QString makePath = Preferences::makePath();
  if (!makePath.isEmpty() && !makePath.endsWith("/"))  // if this is empty, just leave it so the system versions are used
    makePath += "/";
  QStringList args = generateArgs(projectName);
  args.prepend("clean");
  start(QDir::toNativeSeparators(makePath + "make"), args);
}

void Builder::stop()
{
  kill();
}

/*
  This handles the end of each step of the build process.  Maintain which state we're
  in and fire off the next process as appropriate.
*/
void Builder::nextStep(int exitCode, QProcess::ExitStatus exitStatus)
{
  if (exitCode != 0 || exitStatus != QProcess::NormalExit) { // something didn't finish happily
    qDebug() << "err:" << this->errorString();
    mainWindow->onBuildComplete(false);
    resetBuildProcess();
    return;
  }

  switch (buildStep) {
    case BUILD: // the build has just completed.  check the size of the .bin
    {
      QDir dir(currentProjectPath);
      dir.cd("build");
      dir.setNameFilters(QStringList() << "*.bin");
      QFileInfoList bins = dir.entryInfoList();
      bool success = false;
      if (bins.count()) {
        int filesize = bins.first().size();
        if (filesize <= (256 * 1024) ) {
          mainWindow->printOutput(tr("%1 is %2 out of a possible 256K bytes.").arg(bins.first().fileName()).arg(filesize));
          mainWindow->onBuildComplete(true);
          success = true;
        }
      }
      if (!success)
        mainWindow->onBuildComplete(false);
      break;
    }
    case CLEAN:
      mainWindow->onCleanComplete();
      break;
  }
  resetBuildProcess();
}

/*
  Reset the build process.
*/
void Builder::resetBuildProcess()
{
  errMsg.clear();
}

int Builder::getCtrlBoardVersionNumber()
{
  return prefs->ctrlBoardVersion().contains("2.0") ? 200 : 100;
}

int Builder::getAppBoardVersionNumber()
{
  return prefs->appBoardVersion().contains("2.0") ? 200 : 100;
}

/*
  Convert a version number string to 3 ints.
  We expect the version string to be in the form X.Y.Z
*/
bool Builder::parseVersionNumber(int *maj, int *min, int *bld)
{
  QStringList versions = projInfo->version().split(".");
  bool success = false;
  if (versions.count() == 3) {
    int temp = versions.takeFirst().toInt(&success);
    if (success) {
      *maj = temp;
      temp = versions.takeFirst().toInt(&success);
      if (success) {
        *min = temp;
        temp = versions.takeFirst().toInt(&success);
        if (success)
          *bld = temp;
      }
    }
  }
  if (versions.count() != 3 || !success) { // just use the default
    *maj = 0;
    *min = 1;
    *bld = 0;
  }
  return success;
}

void Builder::onBuildError(QProcess::ProcessError error)
{
  QString msg;
  switch (error) {
    case QProcess::FailedToStart:
      msg = tr("'%1' failed to start.  It's either missing, or doesn't have the correct permissions").arg(currentProcess);
      break;
    case QProcess::Crashed:
      msg = tr("'%1' was canceled or crashed.").arg(currentProcess);
      break;
    case QProcess::Timedout:
      msg = tr("'%1' timed out.").arg(currentProcess);
      break;
    case QProcess::WriteError:
      msg = tr("'%1' reported a write error.").arg(currentProcess);
      break;
    case QProcess::ReadError:
      msg = tr("'%1' reported a read error.").arg(currentProcess);
      break;
    case QProcess::UnknownError:
      msg = tr("'%1' - unknown error type.").arg(currentProcess);
      break;
  }
  qDebug() << "err:" << this->errorString();
  mainWindow->printOutputError(tr("Error: ") + msg);
  resetBuildProcess();
  mainWindow->onBuildComplete(false);
}

/*
  Filter the output of the build process
  and only show the most important parts to the user.

  Read line by line...we're only really looking for which file is being compiled at the moment.
*/
void Builder::filterOutput()
{
  // switch based on what part of the build we're performing
  switch(buildStep) {
    case BUILD:
    {
      QString output = readAllStandardOutput();
      buildLog->append(output);
      QTextStream outstream(&output); // use QTextStream to deal with \r\n or \n line endings for us
      QString outline = outstream.readLine();
      while (!outline.isNull()) {
        //qDebug("msg: %s", qPrintable(outline));
        QStringList sl = outline.split(" ");
        if (sl.first().endsWith("arm-none-eabi-gcc") && sl.at(1) == "-c") {
          QFileInfo srcFile(sl.last());
          mainWindow->buildingNow(srcFile.baseName() + ".c");
        }
        outline = outstream.readLine();
      }
      break;
    }
    case CLEAN:
      break;
  }
}

/*
  Filter the error messages of the build process
  and only show the most important parts to the user.

  Try and match the output with some common patterns so we can highlight errors/warnings
  in the editor.
*/
void Builder::filterErrorOutput()
{
  switch(buildStep)
  {
    case BUILD:
    {
      errMsg += readAllStandardError();
      if(!errMsg.endsWith("\n"))
        return;
      buildLog->append(errMsg);
      QTextStream outstream(&errMsg); // use QTextStream to deal with \r\n or \n line endings for us
      QString outline = outstream.readLine();
      bool matched = false;
      while(!outline.isNull()) {
        //qDebug("err: %s", qPrintable(outline));
        QString line = outline;
        outline = outstream.readLine();
        // now try to match the output against common patterns...
        if(matched = matchErrorOrWarning(line))
          continue;
        if(matched = matchInFunction(line))
          continue;
        if(matched = matchUndefinedRef(line))
          continue;
        if(line.startsWith("make")) // don't need to hear anything from make
          continue;
        // last step - we didn't match anything, just print it to the console
        if(!matched)
          mainWindow->printOutputError(line);
      }
      errMsg.clear();
      break;
    }
    case CLEAN:
    {
      QString output = readAllStandardError();
      buildLog->append(output);
      mainWindow->printOutputError(output);
      break;
    }
  }
}

/*
  Try to match an output message from gcc in the form of "filepath:linenumber: error|warning: errormessage"
  If we get one, format it nicely, highlight the line, and pop it into the UI.
*/
bool Builder::matchErrorOrWarning(const QString & error)
{
  bool matched = false;
  QRegExp errExp("([a-zA-Z0-9\\\\/\\.:]+):(\\d+): (error|warning): (.+)");
  int pos = 0;
  while ((pos = errExp.indexIn(error, pos)) != -1) {
    qDebug() << "err" << error;
    QString filepath(errExp.cap(1));
    int linenumber = errExp.cap(2).toInt();
    QString severity(errExp.cap(3));
    QString msg(errExp.cap(4));

    //qDebug("cap! %s: %s, %d - %s", qPrintable(severity), qPrintable(filepath), linenumber, qPrintable(msg));
    QFileInfo fi(filepath);
    QListWidgetItem *item = new QListWidgetItem();
    item->setData(FILEPATH_ROLE, filepath);
    item->setData(LINENO_ROLE, linenumber);;
    QString fullmsg = tr("%1 (line %2): %3").arg(fi.fileName()).arg(linenumber).arg(msg);
    if (severity == "error") {
      item->setData(TYPE_ROLE, ERROR_FEEDBACK);
      item->setIcon(QIcon(":/icons/error.png"));
      mainWindow->highlightLine(filepath, linenumber, ERROR_FEEDBACK);
    }
    else {
      item->setData(TYPE_ROLE, WARNING_FEEDBACK);
      mainWindow->highlightLine(filepath, linenumber, WARNING_FEEDBACK);
      item->setIcon(QIcon(":/icons/warning.png"));
    }
    item->setText(fullmsg);
    mainWindow->printOutputError(item);
    pos += errExp.matchedLength(); // step the index past the match so we can continue looking
    matched = true;
  }
  return matched;
}

/*
  Try to match an output message from gcc in the form of "filepath: In function: msg"
  If we get one, format it nicely, and pop it into the UI.
*/
bool Builder::matchInFunction(const QString & error)
{
  bool matched = false;
  QRegExp errExp("([a-zA-Z0-9\\\\/\\.:]+): In function (.+)");
  int pos;
  while ((pos = errExp.indexIn(error, pos)) != -1) {
    QString filepath(errExp.cap(1));
    QString func(errExp.cap(2));

    //qDebug("cap! %s: In function %s", qPrintable(filepath), qPrintable(func));
    QFileInfo fi(filepath);
    QString fullmsg = tr("%1: In function %2").arg(fi.fileName()).arg(func);
    mainWindow->printOutputError(fullmsg);
    pos += errExp.matchedLength(); // step the index past the match so we can continue looking
    matched = true;
  }
  return matched;
}

/*
  Try to match an output message from gcc in the form of "file: undefined reference to function"
  If we get one, format it nicely, and pop it into the UI.
*/
bool Builder::matchUndefinedRef(const QString & error)
{
  // match output in the form of "filepath: In function: msg"
  bool matched = false;
  QRegExp errExp("(.+):.+: undefined reference to (.+)");
  int pos = 0;
  while ((pos = errExp.indexIn(error, pos)) != -1) {
    QString filepath(errExp.cap(1));
    QString func(errExp.cap(2));

    //qDebug("cap! %s: In function %s", qPrintable(filepath), qPrintable(func));
    QFileInfo fi(filepath);
    QString fullmsg = tr("Error - in %1: Undefined reference to %2").arg(fi.fileName()).arg(func);
    mainWindow->printOutputError(fullmsg);
    pos += errExp.matchedLength(); // step the index past the match so we can continue looking
    matched = true;
  }
  return matched;
}

/*
  Get a list of libraries that the source files in the current project depend on.
  Get a list of the project source files, then scan each one for #include "somelib.h"
  directives and see if any of them match the libs in our libraries directory.
  Create a Library structure for each library and store it in our class member "libraries"
*/
QList<Builder::Library> Builder::loadDependencies(const QString & libsPath, const QString & project)
{
  QDir projDir(project);
  QStringList srcFiles = projDir.entryList(QStringList() << "*.c" << "*.h");
  QDir libDir(libsPath);
  QStringList libDirs = libDir.entryList(QStringList(), QDir::Dirs | QDir::NoDotAndDotDot);
  QList<Library> libraries;

  foreach (QString filename, srcFiles) {
    QFile file(projDir.filePath(filename));
    if (!file.open(QIODevice::ReadOnly|QFile::Text))
      continue;
    // match anything in the form of #include "*.h" or <*.h>
    QRegExp rx("#include [\"|<]([a-zA-Z0-9]*)\\.h[\"|>]");
    QTextStream in(&file);

    while (!in.atEnd()) {
      if (rx.indexIn(in.readLine()) != -1) {
        QString match(rx.cap(1));
        // only list it as a dependency if it's in our list of libraries
        if (libDirs.contains(match)) {
          Library lib;
          lib.name = match;
          // extract the lists of source files specified in the library's spec file
          getLibrarySources(libDir.filePath(match), lib);
          libraries.append(lib);
        }
      }
    }
  }
  return libraries;
}

/*
  Read a library's spec file to determine its source files
  and append those to its file lists.
*/
void Builder::getLibrarySources(const QString & libdir, Library & lib)
{
  QDir dir(libdir);
  QFile libfile(dir.filePath(dir.dirName() + ".xml"));
  QDomDocument libDoc;
  if (libDoc.setContent(&libfile)) {
    QStringList cppSuffixes = QStringList() << "cpp" << "cxx" << "cc";
    QDomNodeList files = libDoc.elementsByTagName("files").at(0).childNodes();
    int filescount = files.count();
    for (int i = 0; i < filescount; i++) {
      QString filepath = dir.filePath(files.at(i).toElement().text());
      QFileInfo fi(filepath);
      if (cppSuffixes.contains(fi.suffix()))
        lib.cppsrc.append(filepath);
      else
        lib.csrc.append(filepath);
    }
  }
}




