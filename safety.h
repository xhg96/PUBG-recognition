#ifndef SAFETY_H
#define SAFETY_H
#include<QThread>
#include<QNetworkAccessManager>
class MSafety:public QObject
{
    Q_OBJECT
public:
    MSafety();
public slots:
    void getMessage(QNetworkReply* reply);

};

#endif // SAFETY_H
