/*
Copyright 2010 Simone Tobia

This file is part of AppSet.

AppSet is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

AppSet is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with AppSet; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <QtGui>
#include <QLibraryInfo>

#include "trayicon.h"

#include <unistd.h>

int main(int argc, char *argv[]){
    sleep(8);

    QApplication app(argc,argv);

    if(app.isSessionRestored())exit(0);

    QApplication::setQuitOnLastWindowClosed(false);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) return 1;

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
         QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    QTranslator myappTranslator;
    myappTranslator.load(":langs/appsettray-qt_" + QLocale::system().name());
         app.installTranslator(&myappTranslator);

    TrayIcon tray;
    //tray.show();

    return app.exec();
}
