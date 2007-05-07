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

#ifndef BOARDLISTMODEL_H_
#define BOARDLISTMODEL_H_

#include <QAbstractListModel>
#include <QStringList>

class BoardListModel : public QAbstractListModel
{
  Q_OBJECT
    
  public:
    
    BoardListModel(const QStringList &strings, QObject *parent = 0);
    ~BoardListModel();
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    
    
    /* QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const; */
                      
  private:
     QStringList stringList;
     
};


#endif /*BOARDLISTMODEL_H_*/
