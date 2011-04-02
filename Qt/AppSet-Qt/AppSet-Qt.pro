#-------------------------------------------------
#
# Project created by QtCreator 2010-11-13T20:20:33
#
#-------------------------------------------------

QT       += core gui network xml webkit

TARGET = appset-qt
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    options.cpp \
    about.cpp

HEADERS  += mainwindow.h \
    options.h \
    about.h

FORMS    += mainwindow.ui \
    options.ui \
    about.ui

INCLUDEPATH += ../../libappset ../libappset-qt
LIBS += -L../../libappset -L../libappset-qt -lappset -lappset-qt

RESOURCES += \
    icons.qrc \
    Langs.qrc

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

TRANSLATIONS = appset-qt_it.ts \
    appset-qt_de.ts \
    appset-qt_nl.ts \
    appset-qt_ca.ts \
    appset-qt_el.ts \
    appset-qt_es.ts \
    appset-qt_fr.ts
