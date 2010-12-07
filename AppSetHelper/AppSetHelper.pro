QT       -= core gui

TARGET = appset-helper
TEMPLATE = app

INCLUDEPATH += ../libappset
LIBS += -L../libappset -lappset

SOURCES += \
    ashelper.cpp

OTHER_FILES += \
    appset-helper.sh

target.path = /usr/bin/
target.files+=appset-helper

INSTALLS += target
