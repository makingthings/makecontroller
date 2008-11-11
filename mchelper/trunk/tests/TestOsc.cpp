/*********************************************************************************

 Copyright 2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

#include "TestOsc.h"

/*
  simple test of creating an OscMessage
*/
void TestOsc::basic()
{
  OscMessage msg;
  osc.createMessage("/test", &msg);
  QVERIFY(msg.data.count() == 0);
  QVERIFY(msg.addressPattern == "/test");
}

/*
  verify that numbers are parsed properly
*/
void TestOsc::numbers()
{
  OscMessage msg;
  osc.createMessage("/test 12 34.5 -3 -23.6", &msg);
  QVERIFY(msg.data.count() == 4);
  QVERIFY(msg.data.at(0)->i == 12);
  QVERIFY(msg.data.at(1)->f == 34.5f);
  QVERIFY(msg.data.at(2)->i == -3);
  QVERIFY(msg.data.at(3)->f == -23.6f);
}

/*
  verify strings are parsed properly
*/
void TestOsc::strings()
{
  OscMessage msg;
  osc.createMessage("/test string \"test spaces\"", &msg);
  QVERIFY(msg.data.count() == 2);
  QVERIFY(msg.data.at(0)->s == "string");
  QVERIFY(msg.data.at(1)->s == "test spaces");
}

/*
  Combine strings and numbers.
*/
void TestOsc::mixed()
{
  OscMessage msg;
  osc.createMessage("/test 23 \"space this\" -34.5 anotherstring 56", &msg);
  QVERIFY(msg.data.count() == 5);
  QVERIFY(msg.data.at(0)->i == 23);
  QVERIFY(msg.data.at(1)->s == "space this");
  QVERIFY(msg.data.at(2)->f == -34.5);
  QVERIFY(msg.data.at(3)->s == "anotherstring");
  QVERIFY(msg.data.at(4)->i == 56);
}

/*
  Create an OscMessage, then confirm that we can
  parse it and come up with the same message.
*/
void TestOsc::roundtrip()
{
  
}







