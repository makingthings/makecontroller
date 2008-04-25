
#include "FindReplace.h"

FindReplace::FindReplace( MainWindow *mw ) : QDialog( )
{
	setupUi(this);
  mainWindow = mw;
  connect( nextButton, SIGNAL(clicked()), this, SLOT(onNext()));
  connect( previousButton, SIGNAL(clicked()), this, SLOT(onPrevious()));
  connect( replaceButton, SIGNAL(clicked()), this, SLOT(onReplace()));
  connect( replaceAllButton, SIGNAL(clicked()), this, SLOT(onReplaceAll()));
  //connect( findBox->lineEdit(), SIGNAL(returnPressed()), this, SLOT(onNext()));
  //connect( replaceBox->lineEdit(), SIGNAL(returnPressed()), this, SLOT(onReplace()));
}

void FindReplace::onNext( )
{
  doFind(true);
}

void FindReplace::onPrevious( )
{
  doFind(false);
}

void FindReplace::doFind(bool forward)
{
  QString findText = findBox->lineEdit()->text();
  if(findText.isEmpty())
    return;
  bool ignoreCase = (ignoreCaseCheckBox->checkState() == Qt::Checked) ? true : false;
  bool wholeWord = (wholeWordCheckBox->checkState() == Qt::Checked) ? true : false;
  mainWindow->findText(findText, ignoreCase, forward, wholeWord);
}

void FindReplace::onReplace( )
{
  
}

void FindReplace::onReplaceAll( )
{
  
}

