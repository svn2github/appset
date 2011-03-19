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
    icons.qrc \
    Langs.qrc

target.path = /usr/bin/
target.files += appsettray-qt

autostart.path = /etc/xdg/autostart/
autostart.files += appsettray-qt.desktop

desktop.path = /usr/share/applications/
desktop.files += appsettray-qt.desktop

INSTALLS += target autostart desktop

OTHER_FILES += \
    appsettray-qt.desktop

TRANSLATIONS = appsettray-qt_it.ts \
    appsettray-qt_de.ts \
    appsettray-qt_nl.ts \
    appsettray-qt_fr.ts \
    appsettray-qt_el.ts \
    appsettray-qt_ca.ts
