
#include "About.h"

/*
 The dialog that pops up when "about mchelper" is selected from the menu.
*/
About::About( ) : QDialog( )
{
	setModal( true );
	setWindowTitle( "About mchelper" );
	topLevelLayout = new QVBoxLayout( this );
  
	okButton = new QPushButton( this );
	okButton->setText( tr("OK") );
	okButton->setFixedWidth( 75 );
	connect( okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	buttonLayout = new QHBoxLayout( );
	buttonLayout->addStretch( );
	buttonLayout->addWidget( okButton );
	buttonLayout->addStretch( );
	
	mchelperIcon = new QPixmap( ":icons/mticon128.png" );
	icon.setPixmap( *mchelperIcon );
	icon.setAlignment( Qt::AlignHCenter );
	
	title.setText( "<font size=5>Make Controller Helper</font>" );
	title.setAlignment( Qt::AlignHCenter );
	version.setText( QString( "<font size=4>Version %1</font>" ).arg( MCHELPER_VERSION ) );
	version.setAlignment( Qt::AlignHCenter );
	description = new QLabel( "<br><b>mchelper</b> (Make Controller Helper) is part of the Make Controller Kit project - an \
    open source hardware platform for everybody.  mchelper can upload new firmware to your Make \
    Controller, and allow you to easily manage it. \
    <br><br> \
    mchelper is released under the <a href=\"http://www.apache.org/licenses/LICENSE-2.0.html\">Apache 2.0 license</a>. \
    <br> \
    Copyright (C) 2006-2008 MakingThings LLC.  <a href=\"http://www.makingthings.com\">www.makingthings.com</a> \
    <br><br> \
    This program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, \
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.", this );
	description->setWordWrap( true );
	description->setFixedWidth( 400 );
	description->setOpenExternalLinks( true );
	
	topLevelLayout->addWidget( &icon );
	topLevelLayout->addWidget( &title );
	topLevelLayout->addWidget( &version );
	topLevelLayout->addWidget( description );
	topLevelLayout->addLayout( buttonLayout );
	topLevelLayout->setAlignment( Qt::AlignHCenter );
}






