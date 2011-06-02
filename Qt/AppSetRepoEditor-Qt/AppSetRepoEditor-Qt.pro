#-------------------------------------------------
#
# Project created by QtCreator 2011-05-25T00:07:18
#
#-------------------------------------------------

QT       += core gui

TARGET = appsetrepoeditor-qt
TEMPLATE = app


SOURCES += main.cpp\
        repoeditor.cpp \
    repoentry.cpp \
    repoconf.cpp \
    checkboxdelegate.cpp \
    addrepo.cpp \
    optionsdelegate.cpp

HEADERS  += repoeditor.h \
    repoentry.h \
    repoconf.h \
    checkboxdelegate.h \
    addrepo.h \
    optionsdelegate.h

FORMS    += repoeditor.ui \
    addrepo.ui

OTHER_FILES += conf/Arch/pacman.repos

target.path = /usr/bin
target.files+=appsetrepoeditor-qt
configuration.path = /etc/appset
configuration.files += conf/*

INSTALLS += target configuration

RESOURCES += \
    icons.qrc \
    Langs.qrc

TRANSLATIONS = appsetrepoeditor-qt_it.ts \
    appsetrepoeditor-qt_de.ts \
    appsetrepoeditor-qt_nl.ts \
    appsetrepoeditor-qt_fr.ts \
    appsetrepoeditor-qt_el.ts \
    appsetrepoeditor-qt_es.ts \
    appsetrepoeditor-qt_ca.ts \
    appsetrepoeditor-qt_pl.ts \
    appsetrepoeditor-qt_pt.ts \
    appsetrepoeditor-qt_zh_TW.ts \
    appsetrepoeditor-qt_zh_CN.ts \
    appsetrepoeditor-qt_tr.ts \
    appsetrepoeditor-qt_ro.ts \
    appsetrepoeditor-qt_sr.ts \
    appsetrepoeditor-qt_*.ts
