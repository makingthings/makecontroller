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

#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include <QAbstractTableModel>
#include "MessageEvent.h"

class TableEntry
{
	public:
		TableEntry( ) { }
		~TableEntry( ) { }

		QString column0, column1, column2;
		MessageEvent::Types type;
};

class OutputWindow : public QAbstractTableModel
{
	Q_OBJECT
	public:
		OutputWindow( QObject *parent = 0 );
		// pure virtuals for QAbstractTableModel
		Qt::ItemFlags flags( const QModelIndex index );
		QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
		QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
		int rowCount( const QModelIndex & parent = QModelIndex() ) const;
		int columnCount( const QModelIndex & parent = QModelIndex() ) const;
		
		void newRows( QList<TableEntry*> entries );
		
	private:
		QList<TableEntry*> tableEntries;
		
	public slots:
		void clear( );
};

#endif // OUTPUTWINDOW_H











