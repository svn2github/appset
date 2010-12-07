#-------------------------------------------------
#
# Project created by QtCreator 2010-11-13T20:20:33
#
#-------------------------------------------------

QT       += core gui network xml webkit

TARGET = appset-qt
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

INCLUDEPATH += ../../libappset ../libappset-qt
LIBS += -L../../libappset -L../libappset-qt -lappset -lappset-qt

RESOURCES += \
    icons.qrc

OTHER_FILES += \
    appset-launch.sh \
    appset-qt.desktop

unix: QMAKE_COPY = "cp -fp"

target.path = /usr/bin/
target.files+=appset-launch.sh appset-qt
images.path = /usr/share/icons/appset
images.files += appset.png
desktop.path = /usr/share/applications
desktop.files += appset-qt.desktop
INSTALLS += target \
    images \
    desktop
