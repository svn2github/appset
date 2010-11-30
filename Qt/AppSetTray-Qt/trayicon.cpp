#include "trayicon.h"

#include <QMenu>

#ifdef unix
#include <asqtnixengine.h>
#endif

#include <QMessageBox>

#include <list>

using namespace std;

TrayIcon::TrayIcon(QObject *parent) :
    QSystemTrayIcon(parent){

    QMenu *trayMenu = new QMenu();

    launch=trayMenu->addAction(QIcon(":general/appset.png"),tr("Launch AppSet-Qt"));

    connect(launch,SIGNAL(triggered()),SLOT(launchAS()));
    connect(this,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),SLOT(activatedSlot(QSystemTrayIcon::ActivationReason)));

    setContextMenu(trayMenu);

    setIcon(QIcon(":general/appset.png"));

#ifdef unix
    as = new AS::QTNIXEngine();
    int errno=0;
    if((errno=((AS::QTNIXEngine*)as)->configure("/etc/appset.conf","/tmp/astray.tmp",true))){
        QMessageBox errMsg;
        errMsg.setText(tr("Error configuring system"));
        errMsg.setInformativeText(((AS::QTNIXEngine*)as)->getConfErrStr(errno));
        errMsg.setIcon(QMessageBox::Critical);
        errMsg.exec();
        exit(1);
    }
#endif

    timer = new QTimer();
    timer->setSingleShot(true);
    timer->start(6666);

    connect(timer,SIGNAL(timeout()),SLOT(checkUps()));
}

void TrayIcon::checkUps(){
    list<Package*> *pkgs = as->queryLocal(as_QUERY_UPGRADABLE);

    if(pkgs->size()){
        QString str("The following upgrades are available\n");
        for(list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
            Package *pkg = *it;
            str+=QString("\n")+pkg->getName().c_str();
        }

        showMessage("Upgrades Check",str,QSystemTrayIcon::Information,66666);
    }else{
        showMessage("Upgrades Check","No upgrades available",QSystemTrayIcon::Information,3333);
    }

    timer->setSingleShot(true);
    timer->start(3600000);
}

void TrayIcon::launchAS(){
    showMessage("Launching AppSet-Qt","You need administrative privileges to run AppSet-Qt...",QSystemTrayIcon::Information,3333);

#ifdef unix
    system("appset-launch.sh &");
#endif
}

void TrayIcon::activatedSlot(QSystemTrayIcon::ActivationReason ar){
    if(ar==QSystemTrayIcon::Trigger) launchAS();
}


TrayIcon::~TrayIcon(){
#ifdef unix
    delete (AS::QTNIXEngine*)as;
#endif
}
