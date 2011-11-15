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
#include "asqtnixengine.h"
#include <QTextStream>

int AS::QTNIXEngine::execCmd(std::string command){
    if(buildBatch){
        (*batchFileStream) << command.c_str() << "\n";

        return 0;
    }

    process = new QProcess();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LANG", "C");
    env.insert("LC_ALL", "C");
    env.insert("COLOR","NO");

    if(!batching){
        process->setProcessEnvironment(env);;

        process->setProcessChannelMode(QProcess::MergedChannels);
    }

    QProcess yesProcess;
    if(!batching)
    if(inputProvider==0 || !inputProvider->isEnabled()){
        yesProcess.setStandardOutputProcess(process);
        yesProcess.start("yes");
    }

    QString cmd(command.c_str());
    cmd = cmd.trimmed();
    QStringList args;
    QString exec;

    int i1=cmd.indexOf('"')-1;
    if(i1>=0){
        int i2=cmd.lastIndexOf('"')+1;
        QString aux(cmd);
        aux.remove(i1,cmd.length());
        args=(aux.split(' '));
        aux=cmd;
        aux.remove(0,i2+1);
        args << cmd.mid(i1+2,i2-i1-3);
        if(aux.length())args<< aux.split(' ');
    }else{
        args=(cmd.split(' '));
    }

    exec=args.at(0);
    args.removeAt(0);

    process->start(exec,args);

    if(batching){
        notifyListeners("Running external X terminal emulator...");

        while(!process->waitForFinished());
        int ret = process->exitCode();

        delete process;
        process = 0;

        return ret;
    }

    int actual=50;
    QString line;
    QTextStream stream;
    stream.setDevice(process);
    while(!process->waitForFinished(actual)){
        if(stopRequested ||
                process->state()==QProcess::NotRunning) break;
        if(!process->canReadLine()) actual=(actual+100)%1000;
        else actual=50;
        QString c;
        while(!(c=stream.read(1)).isEmpty()){
            line+=c;
            if(line.contains('?') || line.contains('\n')){
                notifyListeners(line.trimmed().toAscii().data());
                if(inputProvider && inputProvider->isEnabled()){
                    if(!inputProvider->evaluate(line) && line.contains('?')){
                        stream << "y\n";
                        stream.flush();
                    }
                }
                line.clear();
            }
        }
    }

    if(stopRequested){
        notifyListeners("*** USER STOP REQUEST RECEIVED ***");

        forceStop();
        stopRequested = false;

        return 1;
    }else{

        QString c;
        while(!(c=stream.read(1)).isEmpty()){
            line+=c;
            if(line.contains('?') || line.contains('\n')){
                notifyListeners(line.trimmed().toAscii().data());
                if(inputProvider && inputProvider->isEnabled()){
                    if(!inputProvider->evaluate(line) && line.contains('?')){
                        stream << "y\n";
                        stream.flush();
                    }
                }
                line.clear();
            }
        }

        process->waitForFinished();

        if(inputProvider==0 || !inputProvider->isEnabled()) yesProcess.close();

        int ret=process->exitCode();
        process->kill();
        delete process;

        process=0;

        return ret;
    }
}

void AS::QTNIXEngine::forceStop(){
    if(process && process->state()==QProcess::Running){
        process->kill();
        delete process;
        process=0;

        removeLock();
    }
}

void AS::QTNIXEngine::sendAnswer(QString answer){
    QTextStream stream;
    stream.setDevice(process);
    stream << (answer+"\n");
    stream.flush();
}


#include <QFile>
#include <QTextStream>
int AS::QTNIXEngine::initializeBatch(){
    batchFile = new QFile("/tmp/asbatch.tmp");

    batchFile->open(QIODevice::WriteOnly);

    if(batchFile->isOpen()){
        batchFileStream = new QTextStream(batchFile);

        (*batchFileStream ) << "#!/bin/sh\n";

        buildBatch = true;

        return 0;
    }

    return 1;
}

int AS::QTNIXEngine::finalizeBatch(const QString &pauseMsg){
    (*batchFileStream) << "read -p '" << pauseMsg << "'\n";

    batchFile->close();

    buildBatch = false;

    delete batchFileStream;
    delete batchFile;

    return 0;
}

#include <QStringList>
int AS::QTNIXEngine::executeBatch(const QString &executer){
    QString lowExecuter("sh /tmp/asbatch.tmp");

    batching = true;
    int status = execCmd((QStringList(executer) << lowExecuter).join(" ").toAscii().data());
    batching = false;

    QFile::remove("/tmp/asbatch.tmp");

    return status;
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
        return QObject::tr("It seems that another instance of this program is already running, if sure of not reboot or delete /tmp/as.tmp");
    default:
        break;
    }

    return "";
}

int AS::QTNIXEngine::compareVersions(const QString &s1, const QString &s2){
    if(s1==s2) return 0;

    QStringList l1 = s1.split(QRegExp("[.-+_]")), l2 = s2.split(QRegExp("[.-+_]"));
    bool is1Number = true, is2Number = true;
    int limit = (l1.size() < l2.size()) ? l1.size() : l2.size();

    for(int i=0;i<limit;++i){
        int num1 = l1.at(i).toInt(&is1Number);
        int num2 = l2.at(i).toInt(&is2Number);

        if(is1Number){
            if(!is2Number) return -1;
            else{
                if(num1==num2)continue;
                else if(num1<num2) return -1;
                else return 1;
            }
        }else{
            if(is2Number) return 1;
            else{
                if(l1.at(i)<l2.at(i)) return -1;
                else if(l1.at(i)>l2.at(i)) return 1;
                else continue;
            }
        }
    }

    return s1.length()>s2.length()?1:-1;
}
