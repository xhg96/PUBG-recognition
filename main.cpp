#include "FFVideoPlyer.h"
#include <QtWidgets/QApplication>
#include <QFile>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile file(":qss/style2.qss");
        if(file.open(QFile::ReadOnly))
        {
            QString styleSheet = file.readAll();
            a.setStyleSheet(styleSheet);
            file.close();
        }
    FFVideoPlyer w;

    //w.setStyleSheet(styleSheet);
    w.show();

  // qDebug()<< current_date;//<<mytime;
    return a.exec();
}
