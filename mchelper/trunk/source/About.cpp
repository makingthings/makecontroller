
#include <QDate>
#include "About.h"

/*
 The dialog that pops up when "about mchelper" is selected from the menu.
*/
About::About( ) : QDialog( )
{
  ui.setupUi(this);
  ui.verticalLayout->addWidget(&body);
  body.setOpenExternalLinks(true);
  ui.versionLabel->setText(tr("<font size=6>mchelper</font><br>Version %1").arg(MCHELPER_VERSION));
  body.append(tr("By <a href=\"http://www.makingthings.com\">MakingThings</a>, %1.<br>").arg(QDate::currentDate().toString("yyyy")));
  body.append(tr("Thanks to Erik Gilling for <a href=\"http://oss.tekno.us/sam7utils\">sam7utils</a>.<br>"));
  body.append(trUtf8("French translation by Raphaël Doursenaud."));
}






