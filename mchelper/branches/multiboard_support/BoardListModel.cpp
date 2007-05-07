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
BoardListModel::BoardListModel(const QStringList &strings, QObject *parent) : QAbstractListModel(parent), stringList(strings)
{
  this->stringList = strings;
}


int BoardListModel::rowCount(const QModelIndex &parent) const
{
    return stringList.count();
}

QVariant BoardListModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (index.row() < 0 || index.row() >= stringList.size())
    return QVariant();

  if (role == Qt::DisplayRole)
    return stringList.at(index.row());
  else
    return QVariant();
}
    
BoardListModel::~BoardListModel()
{
}


    
