


#include <QtTest/QtTest>
#include "Osc.h"

/*
 Our test class declaration.
 each slot is automatically called as a test function.
*/
class Test_mchelper: public QObject
{
	Q_OBJECT
  Osc osc;
	private slots:
    void test_OSC_basic();
    void test_OSC_numbers();
    void test_OSC_strings();
    void test_OSC_mixed();
    //void test_OSC_blob(); // eventually specify how to send blobs via text
    void test_OSC_roundtrip();
};

/*
  simple test of creating an OscMessage
*/
void Test_mchelper::test_OSC_basic()
{
  OscMessage msg;
  osc.createMessage("/test", &msg);
  QVERIFY(msg.data.count() == 0);
  QVERIFY(msg.addressPattern == "/test");
}

/*
  verify that numbers are parsed properly
*/
void Test_mchelper::test_OSC_numbers()
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
void Test_mchelper::test_OSC_strings()
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
void Test_mchelper::test_OSC_mixed()
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
void Test_mchelper::test_OSC_roundtrip()
{
  
}

/*
 execute
*/
QTEST_MAIN(Test_mchelper)
#include "testmchelper.moc"




