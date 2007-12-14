/*********************************************************************************

 Copyright 2006-2007 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "OutputWindow.h"
#include <QColor>

OutputWindow::OutputWindow( int maxMsgs, QObject *parent ) : QAbstractTableModel( parent )
{ 
	this->maxMsgs = maxMsgs;
}

Qt::ItemFlags OutputWindow::flags( const QModelIndex index )
{
	(void) index;
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant OutputWindow::data( const QModelIndex & index, int role ) const
{
	if (!index.isValid() || index.row() > tableEntries.count( ) )
		return QVariant();
		
	if( role == Qt::DisplayRole ) // the text that should be written
	{
		switch( index.column( ) )
		{
			case 0:
				return tableEntries.at( index.row() ).column0;
			case 1:
				return tableEntries.at( index.row() ).column1;
			case 2:
				return tableEntries.at( index.row() ).column2;
		}
	}
	
	if( role == Qt::BackgroundRole ) // the background color
	{
		switch( tableEntries.at( index.row() ).type )
		{
			case MessageEvent::Info:
			case MessageEvent::Notice:
				return QColor(235, 235, 235, 235); // light-light grey
			case MessageEvent::Response:
				return QColor( Qt::white );
			case MessageEvent::Error:
				return QColor(255, 221, 221, 255); // Red
			case MessageEvent::Warning:
				return QColor(255, 228, 118, 255); // Orange
			case MessageEvent::Command:
				return QColor(229, 237, 247, 255); // Blue
			case MessageEvent::XMLMessage:
				return QColor(219, 250, 224, 255); // Green
		}
	}
	
	if( role == Qt::ForegroundRole ) // the text color
	{
		if( index.column( ) == 2 ) // timestamp column
			return Qt::gray;
	}
	
	return QVariant( );
}

// add some new rows, but make sure we don't allow more than the specified number of rows
void OutputWindow::newRows( QList<TableEntry> entries )
{
	int newRows = entries.count( );
	int existingRows = tableEntries.count( );
	int requestedRows = newRows + existingRows;
	
	if( requestedRows > maxMsgs )
	{
		// make sure we only remove as many as we need to
		int extraRows = requestedRows - maxMsgs;
		beginRemoveRows( QModelIndex(), 0, extraRows - 1 );
		for( int i = 0; i < extraRows; i++ )
			tableEntries.removeFirst( );
		endRemoveRows( );
	}
	
	beginInsertRows( QModelIndex(), tableEntries.count( ), tableEntries.count( ) + newRows - 1 );
	for( int i = 0; i < newRows; i++ )
		tableEntries.append( entries.at(i) );
	endInsertRows( );
}

void OutputWindow::setMaxMsgs( int newMaxMsgs )
{
	if( newMaxMsgs < maxMsgs )
	{
		if( tableEntries.count( ) > newMaxMsgs )
		{
			int extraRows = tableEntries.count( ) - newMaxMsgs;
			beginRemoveRows( QModelIndex(), 0, extraRows - 1 );
			for( int i = 0; i < extraRows; i++ )
				tableEntries.removeFirst( );
			endRemoveRows( );
		}
	}
	this->maxMsgs = newMaxMsgs;
}

void OutputWindow::clear( )
{
	if( !tableEntries.count( ) > 0 )
		return;
	beginRemoveRows( QModelIndex(), 0, tableEntries.count( ) - 1 );
	tableEntries.clear( );
	endRemoveRows( );
}

QVariant OutputWindow::headerData( int section, Qt::Orientation orientation, int role ) const
{
	(void) section;
	(void) orientation;
	(void) role;
	return QVariant( );
}

int OutputWindow::rowCount( const QModelIndex & parent ) const
{
	(void) parent;
	return tableEntries.count( );
}

int OutputWindow::columnCount( const QModelIndex & parent ) const
{
	(void) parent;
	return 3;
}







