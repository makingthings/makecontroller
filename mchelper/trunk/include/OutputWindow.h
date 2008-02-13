/*********************************************************************************

 Copyright 2006-2008 MakingThings

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

#include <QAbstractItemModel>
#include "MessageEvent.h"

class TableEntry
{
	public:
		TableEntry( QString msg, MessageEvent::Types type, QString tofrom, QString time )
		{
			this->msg = msg;
			this->type = type;
			this->tofrom = tofrom;
			this->timestamp = time;
		}
		~TableEntry( ) { }

		QString msg, tofrom, timestamp;
		MessageEvent::Types type;
};

class OutputWindow : public QAbstractItemModel
{
    Q_OBJECT

	public:
    OutputWindow( int maxMsgs );

    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
		void newRows( QList<TableEntry> entries );
		bool hasChildren( const QModelIndex & parent = QModelIndex() );
		void setMaxMsgs( int newMaxMsgs );
		
	public slots:
		void clear( );

	private:
    int maxMsgs;
		QList<TableEntry> tableEntries;
};

#endif // OUTPUTWINDOW_H











