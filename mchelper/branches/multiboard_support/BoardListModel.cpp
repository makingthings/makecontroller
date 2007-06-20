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

/* Board List Model */
BoardListModel::BoardListModel( QApplication* application, McHelperWindow* mainWindow, QObject *parent  ) : QAbstractListModel(parent)
{
	this->application = application;
	this->mainWindow = mainWindow;
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
    
  const Board *curBoard = (Board*) boardList.at( index.row() );

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
    
    case BoardListModel::NameRole:
     return curBoard->name;
    
    //case BoardListModel::PacketInterfaceRole:
     //return curBoard->packetInterface;
      
    case BoardListModel::COMPortRole:
     return curBoard->com_port;
     
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
   boardList.append(board);
   endInsertRows();

   return boardList.count() - 1;
}

void BoardListModel::removeBoardThreadSafe ( QString key )
{
	BoardEvent* boardEvent = new BoardEvent( key );
	application->postEvent( mainWindow, boardEvent );
}

BoardEvent::BoardEvent( QString string ) : QEvent( (Type)10005 )
{
	message = string;
}

bool BoardListModel::removeBoard ( QString key )
{
  
   // See if we have this board in our list
   int i;
   Board *testBoard;
   for( i = 0; i < boardList.size(); i++ )
   {
       testBoard = boardList.at( i );
       
       // if so, remove it by row id
       if ( testBoard->key == key )
       {
         beginRemoveRows(QModelIndex(), i, i);
         boardList.removeAt(i);
         endRemoveRows();
         // delete boardList.at( i );  need to delete the Board object here, but this crashes it currently
       }
   }
   
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
  // :TODO: implement this
  return true;
}

void BoardListModel::setActiveBoardIndex ( const QModelIndex & index )
{
  this->activeBoard = index;
}

const QModelIndex BoardListModel::getActiveBoardIndex ()
{
  return this->activeBoard;
}

Board* BoardListModel::getActiveBoard ()
{
  Board *curBoard = (Board*) boardList.at( this->activeBoard.row() );
  return curBoard;
}

Qt::DropActions BoardListModel::supportedDropActions() const
{
  return Qt::MoveAction;
}

BoardListModel::~BoardListModel()
{
}



    
