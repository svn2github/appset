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

int AS::QTNIXEngine::compareVersions(const QString &s1, const QString &s2){
    if(s1==s2) return 0;

    QStringList l1 = s1.split('.'), l2 = s2.split('.');
    bool is1Number=true,is2Number=true;
    int limit = l1.size()<l2.size()?l1.size():l2.size();

    for(int i=0;i<limit;++i){
        int num1 = l1.at(i).toInt(&is1Number);
        int num2 = l2.at(i).toInt(&is2Number);

        if(is1Number){
            if(!is2Number) return 1;
            else{
                if(num1==num2)continue;
                else if(num1<num2) return -1;
                else return 1;
            }
        }else{
            if(is2Number) return -1;
            else{
                if(l1.at(i)<l2.at(i)) return -1;
                else if(l1.at(i)>l2.at(i)) return 1;
                else continue;
            }
        }
    }

    return 0;
}
