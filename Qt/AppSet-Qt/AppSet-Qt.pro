#-------------------------------------------------
#
# Project created by QtCreator 2010-11-13T20:20:33
#
#-------------------------------------------------

QT       += core gui network xml webkit declarative

TARGET = appset-qt
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    options.cpp \
    about.cpp \
    appitem.cpp \
    fileitem.cpp \
    filetreemodel.cpp

HEADERS  += mainwindow.h \
    options.h \
    about.h \
    appitem.h \
    fileitem.h \
    filetreemodel.h

FORMS    += mainwindow.ui \
    options.ui \
    about.ui

INCLUDEPATH += ../../libappset ../libappset-qt
LIBS += -L../../libappset -L../libappset-qt -lappset -lappset-qt

RESOURCES += \
    icons.qrc \
    Langs.qrc \
    QML.qrc

OTHER_FILES += \
    appset-launch.sh \
    appset-qt.desktop \
    AppsView.qml \
    lic_template.txt \

unix: QMAKE_COPY = "cp -fp"

target.path = /usr/bin/
target.files+=appset-launch.sh appset-qt
images.path = /usr/share/icons/appset
images.files += appset.png
desktop.path = /usr/share/applications
desktop.files += appset-qt.desktop
autostart.path = /etc/xdg/autostart/
autostart.files += appset-qt.desktop
INSTALLS += target autostart \
    images \
    desktop

TRANSLATIONS = appset-qt_it.ts \
    appset-qt_de.ts \
    appset-qt_nl.ts \
    appset-qt_ca.ts \
    appset-qt_el.ts \
    appset-qt_es.ts \
    appset-qt_fr.ts \
    appset-qt_pl.ts \
    appset-qt_pt.ts \
    appset-qt_pt_BR.ts \
    appset-qt_zh_TW.ts \
    appset-qt_zh_CN.ts \
    appset-qt_tr.ts \
    appset-qt_ro.ts  \
    appset-qt_sr.ts \
    appset-qt_eu.ts \
    appset-qt_ru.ts \
    appset-qt_be.ts \
    appset-qt_hu.ts \
    appset-qt_*.ts




