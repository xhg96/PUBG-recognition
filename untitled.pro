#-------------------------------------------------
#
# Project created by QtCreator 2019-04-07T14:39:05
#
#-------------------------------------------------

QT       += core gui multimedia network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = untitled
TEMPLATE = app
DEFINES+=OPENCV

SOURCES += main.cpp\
    FFVideoPlyer.cpp \
    MyFFmpeg.cpp \
    PlayThread.cpp \
    VideoViewWidget.cpp \
    imgdealthread.cpp \
    mdetector.cpp \
    safety.cpp

HEADERS  += \
    FFVideoPlyer.h \
    MyFFmpeg.h \
    PlayThread.h \
    VideoViewWidget.h \
    imgdealthread.h \
    mdetector.h \
    safety.h

FORMS    += \
    FFVideoPlyer.ui

RESOURCES += \
    FFVideoPlyer.qrc \
    resource.qrc

INCLUDEPATH += \
    E:\cv\opencv\build\include\opencv2\
    E:\cv\opencv\build\include\opencv \
    E:\cv\opencv\build\include\
    E:\cv\darknet\include\
    E:\cv\ffmpeg\include
LIBS+=\
    E:\cv\ffmpeg\lib\avcodec.lib\
    E:\cv\ffmpeg\lib\avdevice.lib\
    E:\cv\ffmpeg\lib\avfilter.lib\
    E:\cv\ffmpeg\lib\avformat.lib\
    E:\cv\ffmpeg\lib\avutil.lib\
    E:\cv\ffmpeg\lib\postproc.lib\
    E:\cv\ffmpeg\lib\swresample.lib\
    E:\cv\ffmpeg\lib\swscale.lib\
    E:\cv\opencv\build\x64\vc15\lib\opencv_world346.lib\
    E:\cv\darknet\lib\yolo_cpp_dll.lib\
    User32.lib

RC_ICONS = logo.ico
