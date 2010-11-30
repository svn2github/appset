QT       += core gui

TARGET = appsettray-qt
TEMPLATE = app

INCLUDEPATH += ../../libappset ../libappset-qt
LIBS += -L../../libappset -L../libappset-qt -lappset -lappset-qt

SOURCES += \
    main.cpp \
    trayicon.cpp

HEADERS += \
    trayicon.h

RESOURCES += \
    icons.qrc
