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
#include <asengine.h>

#ifdef unix
    #include <asnixengine.h>
#endif

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

using namespace AS;

using namespace std;

void handler(int sig){
    if(sig==SIGUSR1) return;
}

int main(){
    Engine *ase;
    int counter = 0;
    int updelay = 60*60;

#ifdef unix
    ofstream pid_writer;
    pid_writer.open("/var/run/ashelper.pid");
    pid_writer << getpid();
    if(pid_writer.is_open()) pid_writer.close();

    struct sigaction sa;
    sa.sa_handler=&handler;
    sa.sa_flags=sa.sa_flags & (~SA_SIGINFO);
    if(sigaction(SIGUSR1,&sa,0)) perror(0);

    ase = new NIXEngine();
    if(((NIXEngine*)ase)->configure("/etc/appset.conf","/tmp/ashelper.tmp")){
        cerr << "Can't start!" << endl;
        exit(1);
    }

    ((NIXEngine*)ase)->removeLock();

    ifstream conf;
    string conf_buffer;
#endif

    ase->update();

    std::list<AS::Package*> *remote, *local;
    while(true){
#ifdef unix
        conf.open("/etc/appset/appset-qt.conf", ifstream::in);
        if(conf.is_open()){
            int i=0;
            while(i!=4 && conf.good()){
                getline (conf,conf_buffer);
                i++;
            }
            updelay = atoi(conf_buffer.data())*60;
        }
        conf.close();
#endif

        local = ase->queryLocal(as_QUERY_ALL_INFO|as_EXPERT_QUERY);
        remote = ase->queryRemote(as_QUERY_ALL_INFO|as_EXPERT_QUERY);

        if(local && remote){
            std::list<AS::Package*>::iterator it=local->begin();

            while(it!=local->end()){
                std::list<AS::Package*>::iterator it2=remote->begin();
                bool found = false;
                Package *pkg = *it;

                while(!found && it2!=remote->end()){
                    Package *pkg2 = *it2;
                    if(pkg2->getName().compare(pkg->getName()) == 0){
                        found = true;
                        pkg2->setLocalVersion(pkg->getLocalVersion());
                        pkg2->setInstalled(true);
                    }
                    it2++;
                }

                if(!found)  remote->insert(remote->end(),pkg);
                else delete pkg;

                it++;
            }

            delete local;

            ofstream output;

#ifdef unix
            output.open("/tmp/ashelper-pre.out");
#endif

            it = remote->begin();
            while(it!=remote->end()){
                Package *pkg=*it;
                output << pkg->getName().c_str() << endl;
                output << pkg->getLocalVersion().c_str() << endl;
                output << pkg->getRemoteVersion().c_str() << endl;
                output << dec << pkg->getSize() << endl;
                output << pkg->getDescription().c_str() << endl;
                output << pkg->getURL().c_str() << endl;
                output << boolalpha << pkg->isInstalled() << endl;
                it++;
                delete pkg;
            }
            if(output.is_open()) output.close();
            delete remote;

#ifdef unix
            rename("/tmp/ashelper-pre.out","/tmp/ashelper.out");
#endif

            bool runned=false;
            int remains = 0;
            while(!runned && remains==0){
                remains = sleep(300);
                counter = (counter+300-remains);
                struct stat s;
                while(stat("/tmp/as.tmp",&s)==0){
                    sleep(5);
                    counter += 5;
                    runned=true;
                }
                if(counter>=updelay){
                    ase->update();
                    counter = 0;
                    runned=true;
                }
            }

#ifdef unix
            unlink("/tmp/ashelper.out");
#endif
        }else{
            sleep(5);
            counter = (counter+5);
        }
    }

    return 0;
}
