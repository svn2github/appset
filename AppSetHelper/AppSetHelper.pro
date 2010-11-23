QT       -= core gui

TARGET = appsethelper
TEMPLATE = app

INCLUDEPATH += ../libappset
LIBS += -L../libappset -lappset

SOURCES += \
    ashelper.cpp
