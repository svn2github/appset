#include "trayicon.h"

#include <QMenu>

#ifdef unix
#include <asqtnixengine.h>
#endif

#include <QMessageBox>

#include <list>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

TrayIcon::TrayIcon(QObject *parent) :
    QSystemTrayIcon(parent){

    manualCheck = false;

    QMenu *trayMenu = new QMenu();

    launch=trayMenu->addAction(QIcon(":general/appset.png"),tr("Launch AppSet-Qt"));
    check=trayMenu->addAction(QApplication::style()->standardIcon(QStyle::SP_BrowserReload),tr("Check for updates NOW!"));
    quit=trayMenu->addAction(QApplication::style()->standardIcon(QStyle::SP_BrowserStop),tr("Quit Tray"));

    connect(launch,SIGNAL(triggered()),SLOT(launchAS()));
    connect(check,SIGNAL(triggered()),SLOT(manualCheckUps()));
    connect(quit,SIGNAL(triggered()),SLOT(quitter()));
    connect(this,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),SLOT(activatedSlot(QSystemTrayIcon::ActivationReason)));

    setContextMenu(trayMenu);

    setIcon(QIcon(":general/appset.png"));

#ifdef unix
    as = new AS::QTNIXEngine();
    int errno=0;
    if((errno=((AS::QTNIXEngine*)as)->configure("/etc/appset.conf","/tmp/astray.tmp",true))){
        /*QMessageBox errMsg;
        errMsg.setText(tr("Error configuring system"));
        errMsg.setInformativeText(((AS::QTNIXEngine*)as)->getConfErrStr(errno));
        errMsg.setIcon(QMessageBox::Critical);
        errMsg.exec();*/
        exit(1);
    }
#endif

    timer = new QTimer();
    timer->setSingleShot(true);
    timer->start(1500);

    timer2 = new QTimer();
    timer2->start(3000);

    connect(timer,SIGNAL(timeout()),SLOT(checkUps()));
    connect(timer2,SIGNAL(timeout()),SLOT(checkRunning()));

    setToolTip(tr("Waiting helper..."));

    running=false;
}

void TrayIcon::manualCheckUps(){
    manualCheck=true;
    //if(!running) showMessage(tr("Checking updates"),tr("Waiting for updates from helper daemon..."),QSystemTrayIcon::Information,6000);
    QCoreApplication::processEvents(QEventLoop::AllEvents,500);
    checkUps();
    manualCheck=false;
}

void TrayIcon::quitter(){
    QMessageBox reqMes;
    reqMes.setText(tr("Are you sure to quit this tray?"));
    reqMes.setInformativeText(tr("You can restart it from desktop menu"));
    reqMes.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    reqMes.setIcon(QMessageBox::Question);
    int res = reqMes.exec();

    if(res==QMessageBox::Yes){
        QCoreApplication::quit();
    }
}

void TrayIcon::checkRunning(){
    int status = 0;

#ifdef unix
    status = system("ls /tmp/as.tmp >/dev/null 2>/dev/null");
#endif

    if(status == 0){
        setIcon(QIcon(":pkgstatus/working.png"));

        setToolTip(tr("AppSet-Qt is Running!"));
    }else if(running){
        checkUps();
    }

    running = status!=0?false:true;
}

bool compareRepos(Package *p1, Package *p2){
    return p1->getRepository().compare(p2->getRepository())<0;
}
#include <cctype>
void TrayIcon::checkUps(){
    bool running = this->running;
    if(!running){
        //list<Package*> *pkgs = as->queryLocal(as_QUERY_UPGRADABLE);
#ifdef unix
        /*ofstream upreq;
        upreq.open("/tmp/asupreq.tmp");
        upreq.write("upreq",6);
        upreq.close();
        int tried=0;
        struct stat s;
        while(stat("/tmp/asupreq.tmp",&s)==0 && tried<6){
            QCoreApplication::processEvents(QEventLoop::AllEvents,500);
            sleep(5);
            tried++;
        }
        tried=0;
        while(stat("/tmp/ashelper.out",&s) && tried<6){
            QCoreApplication::processEvents(QEventLoop::AllEvents,500);
            sleep(5);
            tried++;
        }*/
#endif
        Package *pp=new Package(true);
        pp->setName("");
        std::list<Package*> *pkgs = as->checkDeps(pp,true,true);//as->queryLocal(as_QUERY_UPGRADABLE|as_EXPERT_QUERY);
        if(pkgs && pkgs->size()>1)pkgs->sort(compareRepos);
        delete pp;

        if(pkgs && pkgs->size()){
            QString str=pkgs->size()>1?tr("There are updates for:"):tr("There is an update for:");
            int i=30;
            for(list<Package*>::iterator it=pkgs->begin();i>0 && it!=pkgs->end();it++){
                Package *pkg = *it;
                str+=QString("\n- ")+QString(pkg->getRepository().c_str())+QString("/")+QString(pkg->getName().c_str())+QString(" ")+QString(pkg->getRemoteVersion().c_str());
                i--;
            }

            if(i==0){
                str+="\n";
                str+=tr("\nAnd others...");
            }

            setToolTip(QString::number(pkgs->size())+QString((pkgs->size()>1?tr(" updates available!"):tr(" update available!"))));

            if(manualCheck)
                showMessage(QString::number(pkgs->size())+(pkgs->size()>1?tr(" updates available!"):tr(" update available!")),str,QSystemTrayIcon::Information);
            setIcon(QIcon(":pkgstatus/upgrade.png"));
        }else{
            if(manualCheck)
                showMessage("AppSetTray-Qt",tr("No updates available"),QSystemTrayIcon::Information,3000);
            setIcon(QIcon(":general/appset.png"));

            setToolTip(tr("No updates available"));
        }
    }

    timer->setSingleShot(true);
    ifstream conf;
    string conf_buffer;
    int updelay=60*60*1000;
#ifdef unix
    conf.open("/etc/appset/appset-qt.conf", ifstream::in);
    if(conf.is_open()){
        int i=0;
        while(i!=4 && conf.good()){
            getline (conf,conf_buffer);
            i++;
        }
        updelay = atoi(conf_buffer.data())*60*1000;
    }
    conf.close();
#endif
    timer->start(running?3000:updelay);
}

void TrayIcon::launchAS(){
    showMessage(tr("Launching AppSet-Qt"),tr("Wait..."),QSystemTrayIcon::Information,1000);

#ifdef unix
    system("appset-launch.sh &");
#endif
}

void TrayIcon::activatedSlot(QSystemTrayIcon::ActivationReason ar){
    if(ar==QSystemTrayIcon::Trigger){
        checkRunning();
        if(!running)
            launchAS();
    }
}


TrayIcon::~TrayIcon(){
#ifdef unix
    delete (AS::QTNIXEngine*)as;
#endif
}
