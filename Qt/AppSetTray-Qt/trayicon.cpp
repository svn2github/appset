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
    check=trayMenu->addAction(QApplication::style()->standardIcon(QStyle::SP_BrowserReload),tr("Check for updates NOW!"));
    quit=trayMenu->addAction(QApplication::style()->standardIcon(QStyle::SP_BrowserStop),tr("Quit Tray"));

    connect(launch,SIGNAL(triggered()),SLOT(launchAS()));
    connect(check,SIGNAL(triggered()),SLOT(checkUps()));
    connect(quit,SIGNAL(triggered()),SLOT(quitter()));
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
    timer->start(1500);

    timer2 = new QTimer();
    timer2->start(3000);

    connect(timer,SIGNAL(timeout()),SLOT(checkUps()));
    connect(timer2,SIGNAL(timeout()),SLOT(checkRunning()));

    setToolTip("Waiting helper...");

    running=false;
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

void TrayIcon::checkUps(){
    bool running = this->running;
    if(!running){
        list<Package*> *pkgs = as->queryLocal(as_QUERY_UPGRADABLE);

        if(pkgs && pkgs->size()){
            QString str=pkgs->size()>1?tr("There are updates for:"):tr("There is an update for:");
            int i=30;
            for(list<Package*>::iterator it=pkgs->begin();i>0 && it!=pkgs->end();it++){
                Package *pkg = *it;
                str+=QString("\n- ")+QString(pkg->getName().c_str())+QString(" ")+QString(pkg->getLocalVersion().c_str());
                i--;
            }

            if(i==0) str+="\nAnd others...";

            setToolTip(QString::number(pkgs->size())+QString((pkgs->size()>1?tr(" updates available!"):tr(" update available!"))));

            showMessage(QString::number(pkgs->size())+(pkgs->size()>1?tr(" updates available!"):tr(" update available!")),str,QSystemTrayIcon::Information,66666);
            setIcon(QIcon(":pkgstatus/upgrade.png"));
        }else{
            showMessage("AppSetTray-Qt",tr("No updates available"),QSystemTrayIcon::Information,3000);
            setIcon(QIcon(":general/appset.png"));

            setToolTip(tr("No updates available"));
        }
    }

    timer->setSingleShot(true);
    timer->start(running?3000:1800000);
}

void TrayIcon::launchAS(){
    showMessage("Launching AppSet-Qt","Wait...",QSystemTrayIcon::Information,1000);

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
