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
#include <QtGui/QApplication>

#include <QLocale>
#include <QTranslator>

#include "mainwindow.h"

#include <signal.h>
#include <fstream>

using namespace std;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if(a.isSessionRestored())exit(0);

#ifdef unix
    system("appsettray-qt &");
#endif

    QTranslator myappTranslator;
    myappTranslator.load(":langs/appset-qt_" + QLocale::system().name());
         a.installTranslator(&myappTranslator);

    MainWindow w;
    //w.show();

    ASHider hider(&w);
    hider.start();

    hider.connect(&hider,SIGNAL(hide()),&w,SLOT(hidePriv()));
    hider.connect(&hider,SIGNAL(show()),&w,SLOT(showPriv()));
    hider.connect(&hider,SIGNAL(upDB()),&w,SLOT(addRows()));
    hider.connect(&hider,SIGNAL(quit()),qApp,SLOT(quit()));

    return a.exec();
}
