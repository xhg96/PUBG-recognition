#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_FFVideoPlyer.h"
#include "MyFFmpeg.h"
#include "PlayThread.h"
#include "imgdealthread.h"
#include <QTimer>
class FFVideoPlyer : public QMainWindow
{
    Q_OBJECT

public:
    FFVideoPlyer(QWidget *parent = Q_NULLPTR);
    ~FFVideoPlyer();
public slots:
    void showInfo();

private slots:
    void on_btn_openURL_clicked();

    void on_btn_REC_clicked();

    void on_checkShow_clicked();

    void on_btn_openVideo_clicked();

    void on_btn_openImg_clicked();

    void dectSlot();

    void saveSettings();

    void readSettings();
    void on_btn_UP_clicked();

    void on_btn_Down_clicked();

    void on_btn_Left_clicked();

    void on_btn_Right_clicked();

private:
    MyFFmpeg *m_ffmpeg;
    PlayThread *m_Thread;
    imgDealThread *imgThread;
    QTimer * showTimer;
    Ui::FFVideoPlyerClass *ui;
};
