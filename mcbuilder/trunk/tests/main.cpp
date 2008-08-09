

#include <QtTest/QtTest>
#include "TestProjectManager.h"
#include "TestBuilder.h"

int main(int argc, char** argv)
{  
  (void)argc;
  (void)argv;
  
  TestProjectManager testProjectManager;
	QTest::qExec(&testProjectManager);
  
  TestBuilder testBuilder;
  QTest::qExec(&testBuilder);
}





