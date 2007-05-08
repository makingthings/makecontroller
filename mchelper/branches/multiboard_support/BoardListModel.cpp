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
    
  const Board *curBoard = boardList.at( index.row() );

  if (role == Qt::DisplayRole)
    return curBoard->name;
  else
    return QVariant();
}

bool BoardListModel::setData ( const QModelIndex & idx, const QVariant & value, int role )
{
  return true;
}

BoardListModel::~BoardListModel()
{
}


    
