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
    unlink("/tmp/ashelper.tmp");
    unlink("/tmp/ashelper.out");
    unlink("/tmp/astray.tmp");
    unlink("/tmp/asmin.tmp");
    unlink("/tmp/as.tmp");
    unlink("/tmp/aspriv.tmp");
    unlink("/tmp/asinstall.tmp");
    unlink("/tmp/asremove.tmp");
    unlink("/tmp/asupgrade.tmp");
    unlink("/tmp/asshown.tmp");
    unlink("/tmp/asbatch.tmp");

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
    bool updelaySet = false;
#endif

    ase->update();

    std::list<AS::Package*> *remote, *local;
    while(true){
        local = ase->queryLocal(as_QUERY_ALL_INFO|as_EXPERT_QUERY);
        remote = ase->queryRemote(as_QUERY_ALL_INFO|as_EXPERT_QUERY);

        if(local && remote){
            std::list<AS::Package*>::iterator it=local->begin();

            while(it!=local->end()){
                std::list<AS::Package*>::iterator it2=remote->begin();
                bool found = false;
                Package *pkg = *it;

                while(/*!found && */it2!=remote->end()){
                    Package *pkg2 = *it2;

                    if(pkg2->getName().compare(pkg->getName()) == 0){
                        found = true;
                        pkg2->setLocalVersion(pkg->getLocalVersion().c_str());
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
                output << pkg->getRepository().c_str() << endl;
                it++;
                delete pkg;
            }
            if(output.is_open()) output.close();
            delete remote;

#ifdef unix
            unlink("/tmp/ashelper.out");
            rename("/tmp/ashelper-pre.out","/tmp/ashelper.out");
#endif

            bool runned=false;
            int remains = 0;
            //bool upreq=false;
            while(!runned && remains==0){
                remains = sleep(5);

#ifdef unix
                //Retrieving update delay
                conf.open("/tmp/ashdelay.tmp", ifstream::in);
                //cout << "TRYING TO GET UPDELAY" << endl;
                if(conf.is_open()){
                    getline (conf,conf_buffer);

                    updelay = atoi(conf_buffer.data())*60;

                    //cout << "GOT IT: " << updelay << endl;

                    conf.close();
                    updelaySet = true;
                    unlink("/tmp/ashdelay.tmp");
                }//else cout << "CAN'T GET IT" << updelay << endl;
#endif

                counter = (counter+5-remains);
                struct stat s;
                while(stat("/tmp/as.tmp",&s)==0 || stat("/tmp/asshown",&s)==0){
                    sleep(5);
                    counter += 5;
                    runned=true;
                }
                //upreq=stat("/tmp/asupreq.tmp",&s)==0;
                if((counter>=updelay/* || upreq*/) && stat("/tmp/as.tmp",&s) && stat("/tmp/asshown",&s)){
                    ase->update();
                    if(stat("/tmp/asmin",&s)==0) system("echo -ne 'update\n' > /tmp/asmin");
                    counter = 0;
                    runned=true;                    
                }
            }

#ifdef unix
            unlink("/tmp/ashelper.out");
            /*if(upreq){
                unlink("/tmp/asupreq.tmp");
            }*/
#endif
        }else{
            sleep(5);
            counter = (counter+5);
        }
    }

    return 0;
}
