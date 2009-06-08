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
#include <QtDebug>

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
  QVERIFY(msg.data.at(0)->i() == 12);
  QVERIFY(msg.data.at(1)->f() == 34.5f);
  QVERIFY(msg.data.at(2)->i() == -3);
  QVERIFY(msg.data.at(3)->f() == -23.6f);
}

/*
  verify strings are parsed properly
*/
void TestOsc::strings()
{
  OscMessage msg;
  osc.createMessage("/test string \"test spaces\"", &msg);
  QVERIFY(msg.data.count() == 2);
  QVERIFY(msg.data.at(0)->s() == "string");
  QVERIFY(msg.data.at(1)->s() == "test spaces");
}

/*
  Combine strings and numbers.
*/
void TestOsc::mixed()
{
  OscMessage msg;
  osc.createMessage("/test 23 \"space this\" -34.5 anotherstring 56", &msg);
  QVERIFY(msg.data.count() == 5);
  QVERIFY(msg.data.at(0)->i() == 23);
  QVERIFY(msg.data.at(1)->s() == "space this");
  QVERIFY(msg.data.at(2)->f() == -34.5);
  QVERIFY(msg.data.at(3)->s() == "anotherstring");
  QVERIFY(msg.data.at(4)->i() == 56);
}

/*
  Create an OscMessage, then confirm that we can
  parse it and come up with the same message.
*/
void TestOsc::roundtrip()
{
  OscMessage original;
  osc.createMessage("/roundtrip 23 34 5.6 walrus", &original);
  QByteArray ba = original.toByteArray();
  QList<OscMessage*> msgs = osc.processPacket( ba.data(), ba.size() );
  QCOMPARE(msgs.size(), 1);
  OscMessage* processed = msgs.first();
  QCOMPARE(processed->data.size(), original.data.size());
//  for( int i = 0; i < processed->data.size(); i++ )

  QCOMPARE(processed->data.at(0)->i(), original.data.at(0)->i());
  QCOMPARE(processed->data.at(1)->i(), original.data.at(1)->i());
  QCOMPARE(processed->data.at(2)->f(), original.data.at(2)->f());
  QCOMPARE(processed->data.at(3)->s(), original.data.at(3)->s());
}

/*
  Create a few OscMessages, put them in a bundle
  and make sure we get the right stuff back.
*/
void TestOsc::bundle()
{
  QStringList strings;
  strings << "/test number 1 -2 3";
  strings << "/test2 donkey good";
  strings << "/test3 34.6 twelve";

  QByteArray bundle = osc.createPacket(strings);
  QList<OscMessage*> msgs = osc.processPacket( bundle.data(), bundle.size() );
  QCOMPARE(msgs.size(), strings.size());

  OscMessage* m = msgs.at(0);
  QVERIFY(m->addressPattern == "/test");
  QVERIFY(m->data.at(0)->s() == "number");
  QCOMPARE(m->data.at(1)->i(), 1);
  QCOMPARE(m->data.at(2)->i(), -2);
  QCOMPARE(m->data.at(3)->i(), 3);

  m = msgs.at(1);
  QVERIFY(m->addressPattern == "/test2");
  QVERIFY(m->data.at(0)->s() == "donkey");
  QVERIFY(m->data.at(1)->s() == "good");

  m = msgs.at(2);
  QVERIFY(m->addressPattern == "/test3");
  QVERIFY(m->data.at(0)->f() == 34.6f);
  QVERIFY(m->data.at(1)->s() == "twelve");
}







