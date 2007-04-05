#ifndef UI_CTEST_H
#define UI_CTEST_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QStatusBar>
#include <QtGui/QTextEdit>
#include <QtGui/QWidget>

class Ui_CTest
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QLabel *label;
    QTextEdit *testOutput;
    QLabel *statusLabel;
    QPushButton *goButton;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *CTest)
    {
    CTest->setObjectName(QString::fromUtf8("CTest"));
    CTest->resize(QSize(801, 601).expandedTo(CTest->minimumSizeHint()));
    centralwidget = new QWidget(CTest);
    centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
    gridLayout = new QGridLayout(centralwidget);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(9);
    gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
    label = new QLabel(centralwidget);
    label->setObjectName(QString::fromUtf8("label"));
    QFont font;
    font.setFamily(QString::fromUtf8("MS Shell Dlg 2"));
    font.setPointSize(36);
    font.setBold(false);
    font.setItalic(false);
    font.setUnderline(false);
    font.setWeight(50);
    font.setStrikeOut(false);
    label->setFont(font);
    label->setAlignment(Qt::AlignCenter);

    gridLayout->addWidget(label, 0, 0, 1, 2);

    testOutput = new QTextEdit(centralwidget);
    testOutput->setObjectName(QString::fromUtf8("testOutput"));
    QFont font1;
    font1.setFamily(QString::fromUtf8("MS Shell Dlg 2"));
    font1.setPointSize(18);
    font1.setBold(false);
    font1.setItalic(false);
    font1.setUnderline(false);
    font1.setWeight(50);
    font1.setStrikeOut(false);
    testOutput->setFont(font1);

    gridLayout->addWidget(testOutput, 2, 0, 1, 2);

    statusLabel = new QLabel(centralwidget);
    statusLabel->setObjectName(QString::fromUtf8("statusLabel"));
    QFont font2;
    font2.setFamily(QString::fromUtf8("MS Shell Dlg 2"));
    font2.setPointSize(32);
    font2.setBold(false);
    font2.setItalic(false);
    font2.setUnderline(false);
    font2.setWeight(50);
    font2.setStrikeOut(false);
    statusLabel->setFont(font2);
    statusLabel->setAutoFillBackground(false);
    statusLabel->setAlignment(Qt::AlignCenter);

    gridLayout->addWidget(statusLabel, 1, 1, 1, 1);

    goButton = new QPushButton(centralwidget);
    goButton->setObjectName(QString::fromUtf8("goButton"));
    QFont font3;
    font3.setFamily(QString::fromUtf8("MS Shell Dlg 2"));
    font3.setPointSize(32);
    font3.setBold(false);
    font3.setItalic(false);
    font3.setUnderline(false);
    font3.setWeight(50);
    font3.setStrikeOut(false);
    goButton->setFont(font3);

    gridLayout->addWidget(goButton, 1, 0, 1, 1);

    CTest->setCentralWidget(centralwidget);
    menubar = new QMenuBar(CTest);
    menubar->setObjectName(QString::fromUtf8("menubar"));
    menubar->setGeometry(QRect(0, 0, 801, 25));
    CTest->setMenuBar(menubar);
    statusbar = new QStatusBar(CTest);
    statusbar->setObjectName(QString::fromUtf8("statusbar"));
    CTest->setStatusBar(statusbar);
    retranslateUi(CTest);

    QMetaObject::connectSlotsByName(CTest);
    } // setupUi

    void retranslateUi(QMainWindow *CTest)
    {
    CTest->setWindowTitle(QApplication::translate("CTest", "Controller Tester", 0, QApplication::UnicodeUTF8));
    label->setText(QApplication::translate("CTest", "MAKE CONTROLLER BOARD", 0, QApplication::UnicodeUTF8));
    statusLabel->setText(QApplication::translate("CTest", "OK", 0, QApplication::UnicodeUTF8));
    goButton->setText(QApplication::translate("CTest", "GO", 0, QApplication::UnicodeUTF8));
    Q_UNUSED(CTest);
    } // retranslateUi

};

namespace Ui {
    class CTest: public Ui_CTest {};
} // namespace Ui

#endif // UI_CTEST_H
