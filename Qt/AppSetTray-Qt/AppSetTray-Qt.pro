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
    ../AppSet-Qt/icons.qrc

target.path = /usr/bin/
target.files += appsettray-qt

autostart.path = /etc/xdg/autostart/
autostart.files += appsettray-qt.desktop

desktop.path = /usr/share/applications/
desktop.files += appsettray-qt.desktop

INSTALLS += target autostart desktop

OTHER_FILES += \
    appsettray-qt.desktop
