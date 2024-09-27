#-------------------------------------------------
#
# Project created by QtCreator 2016-01-21T09:54:22
#
#-------------------------------------------------
# git commit rules
# https://habr.com/ru/post/164297/
# https://habr.com/ru/company/mailru/blog/572560/
# https://ru.stackoverflow.com/questions/640787/%D0%A1%D1%82%D0%B8%D0%BB%D1%8C-%D0%BA%D0%BE%D0%BC%D0%BC%D0%B8%D1%82%D0%BE%D0%B2-%D0%B2-git
# https://docs.google.com/document/d/1QrDFcIiPjSLDn3EL15IJygNPiHORgU1_OOAqWjiDU5Y/edit#heading=h.uyo6cb12dt6w
QT += core gui
QT += serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = timerrf
TEMPLATE = app
include (config.pro)
QT += widgets
VERSION = 1.0.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
INCLUDEPATH += ../
MOC_DIR += moc
OBJECTS_DIR +=obj

#IN
SOURCES +=  main.cpp\
            hwBehave.cpp \
            main_timerrf.cpp 


HEADERS  += main_timerrf.h  \
    hwBehave.h

#OUT
DESTDIR = ../$${RELEASEDIR}
#TRANSLATIONS +=rfamp_ru.ts

