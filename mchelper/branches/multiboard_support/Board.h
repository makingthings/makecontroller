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

#ifndef BOARD_H_
#define BOARD_H_

#include <stdarg.h>
#include <stdlib.h>

#include <QString>

class Board
{
  public:
    char* address;
    char* s;
    int   i;
    float f;
    
    Board( );
    
    ~Board()
    {
      if ( address != 0 )
        free( address );
    }
    
    QString name;
      
  private:
      
};

class UdpBoard : public Board
{
  public:
    UdpBoard( );
};

class UsbSerialBoard : public Board
{
  public:
    UsbSerialBoard( );
};

class UsbSambaBoard : public Board
{
  public:
    UsbSambaBoard( );
};



#endif /*BOARD_H_*/
