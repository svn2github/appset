QT       += core gui

TARGET = appset-qt
TEMPLATE = lib
CONFIG += staticlib

unix{
    HEADERS+=asqtnixengine.h inputprovider.h communityrepomodel.h
    SOURCES+=asqtnixengine.cpp inputprovider.cpp communityrepomodel.cpp
}

INCLUDEPATH += ../../libappset
LIBS += -L../../libappset -lappset

RESOURCES += \
    icons.qrc
