#-------------------------------------------------
#
# Project created by QtCreator 2017-02-09T16:43:39
#
#-------------------------------------------------

QT       += core gui
CONFIG += qwt

include ( C:/Qwt-6.1.3/features/qwt.prf )

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = neuro-view
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    fftworker.cpp

HEADERS  += mainwindow.h \
    fftworker.h

FORMS    += mainwindow.ui

win32: LIBS += -L$$PWD/thinkgear/ -lthinkgear

INCLUDEPATH += $$PWD/thinkgear
DEPENDPATH += $$PWD/thinkgear

win32: LIBS += -L$$PWD/fftw/ -llibfftw3-3

INCLUDEPATH += $$PWD/fftw
DEPENDPATH += $$PWD/fftw

win32: LIBS += -L$$PWD/fftw/ -llibfftw3f-3

INCLUDEPATH += $$PWD/fftw
DEPENDPATH += $$PWD/fftw


