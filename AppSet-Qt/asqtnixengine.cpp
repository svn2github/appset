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
#include <QProcess>

#include "asqtnixengine.h"

int AS::QTNIXEngine::execCmd(std::string command){
    QProcess process;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LANG", "en");

    process.setProcessEnvironment(env);;

    process.start(command.c_str());

    while(!process.waitForFinished(1000)){
        if(process.state()==QProcess::NotRunning) break;
        while(process.canReadLine()){
            notifyListeners(QString(process.readLine().data()).trimmed().toAscii().data());
        }
    }

    while(process.canReadLine()){
        notifyListeners(QString(process.readLine().data()).trimmed().toAscii().data());
    }

    process.waitForFinished();

    return process.exitCode();
}

QString AS::QTNIXEngine::getConfErrStr(int errno){
    switch(errno){
    case 1:
        return QObject::tr("Cannot open appset.conf file, check your AppSet configuration");
    case 2:
        return QObject::tr("appset.conf file does not contains all parameters, check your AppSet configuration");
    case 3:case 5:
        return QObject::tr("Cannot open your tool's configuration file, check your AppSet configuration");
    case 4:case 6:
        return QObject::tr("Your tool's configuration file is not complete, check your AppSet configuration");
    case 7:
        return QObject::tr("It seems that you are not an administrator, use sudo, gksu or kdesu to launch AppSet...");
    case 8:
        return QObject::tr("It seems that another instance of this program is already running, if sure of not reboot or delete /tmp/appset.tmp");
    default:
        break;
    }

    return "";
}
