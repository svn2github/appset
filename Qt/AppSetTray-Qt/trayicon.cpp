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

    upgradables = 0;
    visibility = 1; //not visible by default

    priv=0;

    QMenu *trayMenu = new QMenu();

    launch=trayMenu->addAction(QIcon(":general/appset.png"),tr("Show/Hide AppSet"));
    check=trayMenu->addAction(QApplication::style()->standardIcon(QStyle::SP_BrowserReload),tr("Check for updates NOW!"));
    quit=trayMenu->addAction(QApplication::style()->standardIcon(QStyle::SP_BrowserStop),tr("Quit"));

    connect(launch,SIGNAL(triggered()),SLOT(launchAS()));
    connect(check,SIGNAL(triggered()),SLOT(manualCheckUps()));
    connect(quit,SIGNAL(triggered()),SLOT(quitter()));
    connect(this,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),SLOT(activatedSlot(QSystemTrayIcon::ActivationReason)));

    setContextMenu(trayMenu);

    setIcon(QIcon(":general/appset.png"));

#ifdef unix
    as = new AS::QTNIXEngine(new InputProvider());
    int errno=0;
    if((errno=((AS::QTNIXEngine*)as)->configure("/etc/appset.conf","/tmp/astray.tmp",true))){
        exit(1);
    }
#endif

    /*timer = new QTimer();
    timer->setSingleShot(true);
    timer->start(1500);*/

    timer2 = new QTimer();
    timer2->setSingleShot(true);
    timer2->start(3000);

    //connect(timer,SIGNAL(timeout()),SLOT(checkUps()));
    connect(timer2,SIGNAL(timeout()),SLOT(checkRunning()));

    setToolTip(tr("Waiting helper..."));

    running=false;
}

void TrayIcon::manualCheckUps(){
    check->setDisabled(true);
    timer2->stop();
    setIcon(QIcon(":pkgstatus/working.png"));
    setToolTip(tr("AppSet-Qt is Running!"));
    manualCheck=true;
    QCoreApplication::processEvents(QEventLoop::AllEvents,500);
    QStringList args;
    args << "--update";
    priv=new QProcess(this);
    priv->start("appset-launch.sh",args);
    connect(priv,SIGNAL(finished(int)),this,SLOT(manualCheckFinished(int)));
}

void TrayIcon::manualCheckFinished(int s){
    timer2->setSingleShot(true);
    timer2->start(100);
}

#include <QFile>
#include <QTextStream>
void TrayIcon::quitter(){
    checkRunning();
    QMessageBox reqMes;
    int res=QMessageBox::Yes;
    if(running){
        reqMes.setText(tr("There is an instance of AppSet which is running some privileged operations."));
        reqMes.setInformativeText(tr("Do you want to quit anyway (not recommended)?"));
        reqMes.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        reqMes.setIcon(QMessageBox::Warning);
        res = reqMes.exec();
    }

    if(res==QMessageBox::Yes){
        if(QFile::exists("/tmp/asmin")){
            QFile pfile("/tmp/asmin");
            pfile.open(QIODevice::WriteOnly|QIODevice::Text);
            QTextStream aspriv(&pfile);
            aspriv << "quit\n";
            pfile.close();
        }

        QCoreApplication::quit();
    }
}

#include <QFile>
#include <QDesktopServices>
#include <QTextStream>
void TrayIcon::checkRunning(){
    if(priv){
        delete priv;
        priv=0;
    }

    int status = 0;

#ifdef unix
    status = system("ls /tmp/as.tmp >/dev/null 2>/dev/null");
#endif

    running = status!=0?false:true;

    if(QFile::exists("/tmp/asshown") || status==0) check->setDisabled(true);
    else check->setDisabled(false);

    if(status == 0){
        setIcon(QIcon(":pkgstatus/working.png"));

        setToolTip(tr("AppSet-Qt is Running!"));
    }else{
        checkUps();
    }

    //Set visibility from options
    QString confPath(QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
    confPath += "/.appset-qt.conf";
    QFile conf(confPath);
    QString conf_buffer;
    if(conf.exists()){
        conf.open(QIODevice::ReadOnly);

        if(conf.isOpen()){
            int i=0;
            QTextStream confStream(&conf);
            while(i!=14 && !confStream.atEnd()){
                conf_buffer = confStream.readLine();
                i++;
            }
            bool isInt=false;
            conf_buffer.toInt(&isInt);
            if(isInt){
                visibility = conf_buffer.toInt();
            }
        }
    }
    conf.close();


    if(visibility && !upgradables && !running && !QFile::exists("/tmp/asshown")){
        this->hide();
    }else{
        this->show();
    }

    timer2->start(3000);
}

bool compareRepos(Package *p1, Package *p2){
    return p1->getRepository().compare(p2->getRepository())<0;
}

#include <cctype>
void TrayIcon::checkUps(){
    bool running = this->running;

    if(!running){
        Package *pp=new Package(true);
        pp->setName("");
        std::list<Package*> *pkgs = as->checkDeps(pp,true,true);//as->queryLocal(as_QUERY_UPGRADABLE|as_EXPERT_QUERY);
        if(pkgs && pkgs->size()>1)pkgs->sort(compareRepos);
        delete pp;

        if(pkgs && pkgs->size()){
            QString str=pkgs->size()>1?tr("There are updates for:"):tr("There is an update for:");
            int i=30;
            for(list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                Package *pkg = *it;
                if(i>0){
                    str+=QString("\n- ")+QString(pkg->getRepository().c_str())+QString("/")+QString(pkg->getName().c_str())+QString(" ")+QString(pkg->getRemoteVersion().c_str());
                }
                i--;

                delete (*it);
            }

            if(i==0){
                str+="\n";
                str+=tr("\nAnd others...");
            }

            upgradables = pkgs->size();

            setToolTip(QString::number(upgradables)+QString((upgradables>1?tr(" updates available!"):tr(" update available!"))));

            if(manualCheck)
                showMessage(QString::number(upgradables)+(upgradables>1?tr(" updates available!"):tr(" update available!")),str,QSystemTrayIcon::Information);
            setIcon(QIcon(":pkgstatus/upgrade.png"));

            pkgs->clear();
            delete pkgs;
        }else{
            if(manualCheck)
                showMessage("AppSetTray-Qt",tr("No updates available"),QSystemTrayIcon::Information,3000);
            setIcon(QIcon(":general/appset.png"));

            setToolTip(tr("No updates available"));

            upgradables = 0;
        }        

    }

    if(manualCheck && !running){
        QFile pfile("/tmp/asmin");
        pfile.open(QIODevice::WriteOnly|QIODevice::Text);
        QTextStream aspriv(&pfile);
        aspriv << "update\n";
        pfile.close();
        manualCheck=false;
    }
}

void TrayIcon::launchAS(){    
#ifdef unix
    system("appset-launch.sh &");
#endif
}

void TrayIcon::activatedSlot(QSystemTrayIcon::ActivationReason ar){
    if(ar==QSystemTrayIcon::Trigger){                
        launchAS();
    }
}


TrayIcon::~TrayIcon(){
#ifdef unix
    delete (AS::QTNIXEngine*)as;
#endif
}
