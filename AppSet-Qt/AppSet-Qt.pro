#-------------------------------------------------
#
# Project created by QtCreator 2010-11-13T20:20:33
#
#-------------------------------------------------

QT       += core gui webkit

TARGET = appset-qt
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

unix{
    SOURCES += asqtnixengine.cpp
    HEADERS += asqtnixengine.h
}

FORMS    += mainwindow.ui

INCLUDEPATH += ../libappset
LIBS += -L../libappset -lappset

RESOURCES += \
    icons.qrc

OTHER_FILES += \
    appset-launch.sh

unix: QMAKE_COPY = "cp -fp"

target.path = /usr/bin/
target.files+=appset-launch.sh appset-qt
target.commands =
images.path = /usr/share/icons/appset
images.files += appset.png
desktop.path = /usr/share/applications
desktop.files += appset-qt.desktop
INSTALLS += target \
    images \
    desktop
