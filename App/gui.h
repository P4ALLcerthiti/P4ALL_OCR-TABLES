#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *buttonLoad;
    QPushButton *buttonShow;
    QLineEdit *lineFilename;
    QTextBrowser *browserDeb;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->setEnabled(true);
        MainWindow->setFixedSize(466,204);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        buttonLoad = new QPushButton(centralwidget);
        buttonLoad->setObjectName(QStringLiteral("buttonLoad"));
        buttonLoad->setGeometry(QRect(10, 10, 75, 23));
        buttonShow = new QPushButton(centralwidget);
        buttonShow->setObjectName(QStringLiteral("buttonShow"));
        buttonShow->setEnabled(false);
        buttonShow->setGeometry(QRect(100, 10, 75, 23));
        lineFilename = new QLineEdit(centralwidget);
        lineFilename->setObjectName(QStringLiteral("lineFilename"));
        lineFilename->setEnabled(false);
        lineFilename->setGeometry(QRect(10, 50, 441, 20));
        browserDeb = new QTextBrowser(centralwidget);
        browserDeb->setObjectName(QStringLiteral("browserDeb"));
        browserDeb->setGeometry(QRect(10, 90, 441, 91));
        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "OCR App", 0));
        buttonLoad->setText(QApplication::translate("MainWindow", "LOAD", 0));
        buttonShow->setText(QApplication::translate("MainWindow", "SHOW", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

