/*********************************************************************************

 Copyright 2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/


#include "About.h"

/*
 The static dialog that pops up when "about mcbuilder" is selected from the menu.
*/
About::About( ) : QDialog( )
{
	setModal( true );
	setWindowTitle( "About mcbuilder" );
	topLevelLayout = new QVBoxLayout( this );
  
	okButton = new QPushButton( this );
	okButton->setText( tr("OK") );
	connect( okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
	buttonLayout = new QHBoxLayout( );
	buttonLayout->addStretch( );
	buttonLayout->addWidget( okButton );
	buttonLayout->addStretch( );
	
	mcbuilderIcon = new QPixmap( ":/icons/mticon128.png" );
	icon.setPixmap( *mcbuilderIcon );
	icon.setAlignment( Qt::AlignHCenter );
	
	title.setText( "<font size=5>Make Controller Builder</font>" );
	title.setAlignment( Qt::AlignHCenter );
	version.setText( QString( "<font size=4>Version %1</font>" ).arg( MCBUILDER_VERSION ) );
	version.setAlignment( Qt::AlignHCenter );
	description = new QLabel( "<br><b>mcbuilder</b> (Make Controller Builder) is part of the Make Controller Kit project - an \
    open source hardware platform for everybody.  mcbuilder makes it easy to write programs and upload them to your \
    Make Controller. \
    <br><br> \
    mcbuilder is released under the <a href=\"http://www.apache.org/licenses/LICENSE-2.0.html\">Apache 2.0 license</a>. \
    <br> \
    Copyright (C) 2006-2008 MakingThings LLC.  <a href=\"http://www.makingthings.com\">www.makingthings.com</a> \
    <br><br> \
    This program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, \
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.", this );
	description->setWordWrap( true );
	description->setOpenExternalLinks( true );
	
	topLevelLayout->addWidget( &icon );
	topLevelLayout->addWidget( &title );
	topLevelLayout->addWidget( &version );
	topLevelLayout->addWidget( description );
	topLevelLayout->addLayout( buttonLayout );
	topLevelLayout->setAlignment( Qt::AlignHCenter );
}






