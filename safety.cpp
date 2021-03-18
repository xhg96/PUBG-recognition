
#include<QNetworkAccessManager>
#include<QNetworkReply>
#include"safety.h"
MSafety::MSafety()
{
    QNetworkAccessManager *accessManager = new QNetworkAccessManager(this);
    connect(accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(getMessage(QNetworkReply*)));
    QNetworkRequest request;
    request.setUrl(QUrl("http://47.103.57.87"));
    accessManager->post(request,QByteArray("pubg"));
}
void MSafety::getMessage(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError)
    {
        qDebug()<<"exit by remote";
        exit(2769);
    }
    else {
//        this->quit();
    }
}
