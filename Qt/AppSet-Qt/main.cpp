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

MainWindow *w;

void handler(int sig){
    if(sig==SIGUSR1){
        if(w->isVisible() && !w->inModal)w->hide();
        else w->show();
        return;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator myappTranslator;
    myappTranslator.load(":langs/appset-qt_" + QLocale::system().name());
         a.installTranslator(&myappTranslator);

    w=new MainWindow();
    w->show();

    ofstream pid_writer;
    pid_writer.open("/var/run/appset.pid");
    pid_writer << getpid();
    if(pid_writer.is_open()) pid_writer.close();

    struct sigaction sa;
    sa.sa_handler=&handler;
    sa.sa_flags=sa.sa_flags & (~SA_SIGINFO);
    if(sigaction(SIGUSR1,&sa,0)) perror(0);

    a.exec();

    delete w;
}
