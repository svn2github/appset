QT       -= core gui

TARGET = appset
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    aspackage.cpp


HEADERS += asengine.h \
    aspackage.h


unix{
    HEADERS+=asnixengine.h
    SOURCES+=asnixengine.cpp
}

OTHER_FILES += \
    TODO.txt
