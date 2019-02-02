#-------------------------------------------------
#
# Project created by QtCreator 2015-08-24T18:41:19
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SegyViewer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    csegyread.cpp \
    util.cpp \
    zoomer.cpp

HEADERS  += mainwindow.h \
    csegyread.h \
    util.h \
    Zoomer.h

FORMS    += mainwindow.ui



LIBS+=-L "C:\Qt\Qt5.10.1\5.10.1\mingw53_32\lib"-lqwt
INCLUDEPATH+=C:\Qt\Qt5.10.1\5.10.1\mingw53_32\include\QWT
include(C:\qwt-6.1.3\qwt.prf)

RESOURCES += \
    res.qrc



RC_FILE = segyviewer.rc
