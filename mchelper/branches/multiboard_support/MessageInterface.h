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

#ifndef MESSAGEINTERFACE_H
#define MESSAGEINTERFACE_H

class MessageInterface
{		
	public:
	  virtual void message( int level, char* format, ... ) = 0;
	  virtual void message( QString string ) = 0;
	  virtual void messageThreadSafe( QString string ) = 0;
	  virtual void sleepMs( int ms ) = 0;
	  virtual void progress( int value ) = 0;
};

#endif
