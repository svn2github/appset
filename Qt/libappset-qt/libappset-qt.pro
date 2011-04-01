QT       += core gui

TARGET = appset-qt
TEMPLATE = lib
CONFIG += staticlib

unix{
    HEADERS+=asqtnixengine.h
    SOURCES+=asqtnixengine.cpp
}

INCLUDEPATH += ../../libappset
LIBS += -L../../libappset -lappset

HEADERS += \
    communityrepomodel.h

SOURCES += \
    communityrepomodel.cpp

RESOURCES += \
    icons.qrc
