#-------------------------------------------------
#
# Project created by QtCreator 2018-01-19T09:55:36
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets serialport
}
else {
    CONFIG += serialport
}

TARGET = RangeShifterControl
TEMPLATE = app


SOURCES += main.cpp \
    mainwindow.cpp \
    open62541.c \
    opcuaclient.cpp \
    opcuaclientdialog.cpp

HEADERS  += mainwindow.h \
    defines.h \
    open62541.h \
    opcuaclient.h \
    opcuaclientdialog.h

FORMS    += mainwindow.ui \
    opcuaclientdialog.ui

RESOURCES += \
    RangeShifterControl.qrc

TRANSLATIONS += RangeShifterControl_ru.ts
