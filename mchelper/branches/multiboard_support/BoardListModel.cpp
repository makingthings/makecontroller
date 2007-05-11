/*********************************************************************************

 Copyright 2006 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "BoardListModel.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include <stdlib.h>


/* Board List Model */
BoardListModel::BoardListModel(const QList<Board*> boards, QObject *parent) : QAbstractListModel(parent), boardList(boards)
{
  this->boardList = boards;
}


int BoardListModel::rowCount(const QModelIndex &parent) const
{
    return boardList.count();
}

QVariant BoardListModel::data(const QModelIndex &index, int role) const
{
  if ( !index.isValid() || index.model() != this )
    return QVariant();

  if (index.row() < 0 || index.row() >= boardList.size())
    return QVariant();
    
  const UsbSerialBoard *curBoard = (UsbSerialBoard*) boardList.at( index.row() );

  QString tmp_string;
  
  switch( role ) {
    case Qt::DisplayRole:
      tmp_string.append(curBoard->name);
      tmp_string.append("::");
      tmp_string.append(curBoard->com_port);
      
      return tmp_string;
      break;
    
    case Qt::ToolTipRole:
      tmp_string.append(curBoard->name);
      tmp_string.append("::");
      tmp_string.append(curBoard->com_port);
      
      return tmp_string;
      break;
    
    case Qt::StatusTipRole:
      tmp_string.append(curBoard->name);
      tmp_string.append("::");
      tmp_string.append(curBoard->com_port);
      
      return tmp_string;
      break;
      
    default:
      return QVariant();
  }
  
  /*
  static QIcon folder(QPixmap(":/images/folder.png"));

  if (role == Qt::DecorationRole)
  return qVariantFromValue(folder);
  */
}

int BoardListModel::addBoard ( Board *board )
{
   beginInsertRows(QModelIndex(), boardList.count(), boardList.count());
   this->boardList.append(board);
   endInsertRows();

   return boardList.count() - 1;
}

bool BoardListModel::removeBoard ( int row, const QModelIndex &parent )
{
   beginRemoveRows(QModelIndex(), row, row);
   this->boardList.removeAt(row);
   endRemoveRows();

   return true;
}

Qt::ItemFlags BoardListModel::flags ( const QModelIndex & index ) const
{
  Qt::ItemFlags empty;
  if( !index.isValid() || index.model() != this )
    return empty;

  Qt::ItemFlags basicFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
  if( true )
    return basicFlags; // Read-only
  else
    return basicFlags | Qt::ItemIsEditable;
}

bool BoardListModel::setData ( const QModelIndex & idx, const QVariant & value, int role )
{
  return true;
}

Qt::DropActions BoardListModel::supportedDropActions() const
{
  return Qt::MoveAction;
}

BoardListModel::~BoardListModel()
{
}


    
