#include "FFVideoPlyer.h"
#include <QFileDialog>
#include <QMessageBox>
#include <iostream>
#include <list>
#include "MyFFmpeg.h"
#include <WinUser.h>
#include <QMenuBar>
#include <QTextCodec>
#include <QTextStream>
#include <QtCore>
#include <QSound>
#include<QRadioButton>
#include"safety.h"
#pragma comment(lib, "User32.lib")
using namespace std;
vector<QRadioButton*> radioGroup;
FFVideoPlyer::FFVideoPlyer(QWidget *parent)
    : QMainWindow(parent),ui(new Ui::FFVideoPlyerClass)
{
    ui->setupUi(this);
 //   QStatusBar* status = new QStatusBar(this);
    MSafety *safe=new MSafety();
    m_ffmpeg=new MyFFmpeg();
    m_Thread=new PlayThread(m_ffmpeg);
    m_Thread->start();
    imgThread=new imgDealThread(m_ffmpeg,ui->spinBoxFPS,ui->checkDeal);
    imgThread->start();
    showTimer=new QTimer();
    connect(showTimer, SIGNAL(timeout()), this, SLOT(showInfo()));
    connect(imgThread,SIGNAL(dectSignal()),this,SLOT(dectSlot()));
    showTimer->start(250);
    ui->openGLWidget->setImgTread(imgThread);
#ifndef MDEBUG
    ui->btn_openImg->setVisible(false);
    ui->btn_openVideo->setVisible(false);
    ui->btn_REC->setVisible(false);
#endif
    radioGroup.push_back(ui->radioButton_1);
    radioGroup.push_back(ui->radioButton_2);
    radioGroup.push_back(ui->radioButton_3);
    radioGroup.push_back(ui->radioButton_4);
    radioGroup.push_back(ui->radioButton_5);
    radioGroup.push_back(ui->radioButton_6);
    radioGroup.push_back(ui->radioButton_7);
    radioGroup.push_back(ui->radioButton_8);
    radioGroup.push_back(ui->radioButton_9);
    QMenu *serralMenu = menuBar()->addMenu(QString::fromLocal8Bit("选项"));
    QAction *actionSave = new QAction(QStringLiteral("保存当前设置"), this);
    serralMenu->addAction(actionSave);
    connect(actionSave, SIGNAL(triggered()), this, SLOT(saveSettings()));
    readSettings();
}
FFVideoPlyer::~FFVideoPlyer()
{
//    delete showTimer;
//    delete m_Thread;
//    delete imgThread;
//    delete m_ffmpeg;不要加析构函数，否者无法彻底退出程序
//    delete ui;
}
void FFVideoPlyer::on_btn_openURL_clicked()
{
    QString url=ui->lineEditURL->text();
    if (url.isEmpty())
    {
        return;
    }

//    if(m_ffmpeg->m_isPlay)
//    {
//        disconnect(showTimer, SIGNAL(timeout()), this, SLOT(showInfo()));
//        delete m_ffmpeg;
//        delete m_Thread;
//        delete imgThread;
//        delete showTimer;
//        m_ffmpeg=new MyFFmpeg();
//        m_Thread=new PlayThread(m_ffmpeg);
//        m_Thread->start();
//        imgThread=new imgDealThread(m_ffmpeg);
//        imgThread->start();
//        showTimer=new QTimer();
//        connect(showTimer, SIGNAL(timeout()), this, SLOT(showInfo()));
//        showTimer->start(250);
//    }
    if(m_ffmpeg->OpenVideo( url.toLocal8Bit().data() ) ==0)
    {
        ui->btn_openURL->setEnabled(false);
        ui->spinBoxFPS->setMaximum((int)(m_ffmpeg->m_FPS.num/m_ffmpeg->m_FPS.den));
        imgThread->spotP.x=m_ffmpeg->m_width*BLOOD_X;
        imgThread->spotP.y=m_ffmpeg->m_height*BLOOD_Y;
    }

}
void FFVideoPlyer::showInfo()
{
    if(m_ffmpeg->m_FPS.den)
        ui->label_FPS->setText(QString::fromUtf8("FPS:")+QString::number((double)(m_ffmpeg->m_FPS.num/m_ffmpeg->m_FPS.den)));
    ui->label_cache->setText(QString::fromLocal8Bit("缓存帧:")+QString::number(m_ffmpeg->rgbList.size()));
}

void FFVideoPlyer::on_btn_REC_clicked()
{
    if(imgThread==NULL) return;
    if(ui->btn_REC->text()=="REC")
    {
        imgThread->saveFlag=true;
        ui->btn_REC->setText("stop");
    }
    else if(ui->btn_REC->text()=="stop")
    {
        imgThread->saveFlag=false;
        ui->btn_REC->setText("REC");
    }
}

void FFVideoPlyer::on_checkShow_clicked()
{
    imgThread->pushShowPic=ui->checkShow->checkState();
}

void FFVideoPlyer::on_btn_openVideo_clicked()
{

    QString filename = QFileDialog::getOpenFileName(this,tr("Open video"),"..", tr("Video File (*.flv *.rmvb *.avi)"));
    if (filename.isEmpty())
        return;
    int ret=m_ffmpeg->OpenVideo( filename.toLocal8Bit().data() );

    if(!ret)
    {
        imgThread->spotP.x=m_ffmpeg->m_width*BLOOD_X;
        imgThread->spotP.y=m_ffmpeg->m_height*BLOOD_Y;
    }
}

void FFVideoPlyer::on_btn_openImg_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "..", tr("Image File (*.jpg *.png *.bmp)"));
    if (filename.isEmpty())
        return;
    std::string name = string((const char *)filename.toLocal8Bit());
    imgThread->picDetect(name);
}
void FFVideoPlyer::dectSlot()
{
//    char key;
//    if(ui->radioButton_1->isChecked())
//        key='1';
//    else if(ui->radioButton_2->isChecked())
//        key='2';
//    else if(ui->radioButton_3->isChecked())
//        key='3';
//    else if(ui->radioButton_4->isChecked())
//        key='4';
//    else if(ui->radioButton_5->isChecked())
//        key='5';
//    else if(ui->radioButton_6->isChecked())
//        key='6';
//    else if(ui->radioButton_7->isChecked())
//        key='7';
//    else if(ui->radioButton_8->isChecked())
//        key='8';
//    else if(ui->radioButton_9->isChecked())
//        key='9';
//    keybd_event(key,0,0,0);
//    keybd_event(key,0,KEYEVENTF_KEYUP,0);
    QSound::play(":/res/sound/8859.wav");
}
void FFVideoPlyer::saveSettings()
{
   // QSound::play(LOSE_SOUND);
    QFile file("param.info");
    if (file.open(QFile::WriteOnly))//|QFile::Text
    {
        QTextStream out(&file);
        out << ui->lineEditURL->text() << endl;
        out << ui->spinBoxFPS->value() << endl;
        out << ui->radioButton_1->isChecked()<<endl;
        out << ui->radioButton_2->isChecked()<<endl;
        out << ui->radioButton_3->isChecked()<<endl;
        out << ui->radioButton_4->isChecked()<<endl;
        out << ui->radioButton_5->isChecked()<<endl;
        out << ui->radioButton_6->isChecked()<<endl;
        out << ui->radioButton_7->isChecked()<<endl;
        out << ui->radioButton_8->isChecked()<<endl;
        out << ui->radioButton_9->isChecked()<<endl;
        file.close();
    }
}
void FFVideoPlyer::readSettings()
{
    QFile file("param.info");
    if (file.open(QFile::ReadOnly))//|QFile::Text
    {
        QString url=(QString)(file.readLine());
        if(!url.isEmpty())
            url.remove(url.size()-1,1);
        ui->lineEditURL->setText(url);
        ui->spinBoxFPS->setValue(((QString)(file.readLine())).toInt());
        if(((QString)(file.readLine())).toInt())
            ui->radioButton_1->setChecked(true);
        if(((QString)(file.readLine())).toInt())
            ui->radioButton_2->setChecked(true);
        if(((QString)(file.readLine())).toInt())
            ui->radioButton_3->setChecked(true);
        if(((QString)(file.readLine())).toInt())
            ui->radioButton_4->setChecked(true);
        if(((QString)(file.readLine())).toInt())
            ui->radioButton_5->setChecked(true);
        if(((QString)(file.readLine())).toInt())
            ui->radioButton_6->setChecked(true);
        if(((QString)(file.readLine())).toInt())
            ui->radioButton_7->setChecked(true);
        if(((QString)(file.readLine())).toInt())
            ui->radioButton_8->setChecked(true);
        if(((QString)(file.readLine())).toInt())
            ui->radioButton_9->setChecked(true);
        file.close();
    }
}

void FFVideoPlyer::on_btn_UP_clicked()
{
    if(imgThread->spotP.y>0)
        --imgThread->spotP.y;
}

void FFVideoPlyer::on_btn_Down_clicked()
{
    if(imgThread->spotP.y<m_ffmpeg->m_height-1)
        ++imgThread->spotP.y;
}

void FFVideoPlyer::on_btn_Left_clicked()
{
    if(imgThread->spotP.x>0)
        --imgThread->spotP.x;
}

void FFVideoPlyer::on_btn_Right_clicked()
{
    if(imgThread->spotP.x<m_ffmpeg->m_width-1)
        ++imgThread->spotP.x;
}
