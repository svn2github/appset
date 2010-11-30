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
#include "mainwindow.h"
#include "ui_mainwindow.h"

#ifdef unix
#include "asqtnixengine.h"
#endif

#include  <QVBoxLayout>

using namespace AS;

class QTEventFilter:public QObject{
protected:
    bool eventFilter(QObject *obj, QEvent *ev){
        if(!obj) return false;
        if(ev->type()==QEvent::MouseButtonDblClick ||
           ev->type()==QEvent::MouseButtonPress ||
           ev->type()==QEvent::MouseButtonRelease ||
           ev->type()==QEvent::KeyPress ||
           ev->type()==QEvent::KeyRelease) return true;
        return false;
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow){    

    ui->setupUi(this);

    QStringList headers;
    headers.append(tr("S"));
    headers.append(tr("Packet"));
    headers.append(tr("Installed Version"));
    headers.append(tr("Last Version"));
    headers.append(tr("Description"));
    ui->tableWidget->setColumnWidth(0,24);
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    QStringList installHeaders;
    installHeaders.append(tr("S"));
    installHeaders.append(tr("Packet"));
    installHeaders.append(tr("Version"));
    installHeaders.append(tr("Dependencies"));
    installHeaders.append(tr("Download Size(MB)"));
    installHeaders.append(tr("Progress"));
    ui->tableInstall->setColumnWidth(0,24);
    ui->tableInstall->setHorizontalHeaderLabels(installHeaders);

    QStringList updateHeaders;
    updateHeaders.append(tr("S"));
    updateHeaders.append(tr("Packet"));
    updateHeaders.append(tr("Installed Version"));
    updateHeaders.append(tr("Last Version"));
    //updateHeaders.append(tr("Upgrade Dependencies"));
    updateHeaders.append(tr("Download Size(MB)"));
    updateHeaders.append(tr("Progress"));
    ui->tableUpgraded->setColumnWidth(0,24);
    ui->tableUpgraded->setHorizontalHeaderLabels(updateHeaders);

    QStringList removeHeaders;
    removeHeaders.append(tr("S"));
    removeHeaders.append(tr("Packet"));
    removeHeaders.append(tr("Required by"));
    removeHeaders.append(tr("Description"));
    ui->tableRemoved->setColumnWidth(0,24);
    ui->tableRemoved->setHorizontalHeaderLabels(removeHeaders);

    ui->editConfirm->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    ui->editCancel->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));

#ifdef unix
    as = new AS::QTNIXEngine();
    int errno=0;
    if((errno=((AS::QTNIXEngine*)as)->configure("/etc/appset.conf"))){
        QMessageBox errMsg;
        errMsg.setText(tr("Error configuring system"));
        errMsg.setInformativeText(((AS::QTNIXEngine*)as)->getConfErrStr(errno));
        errMsg.setIcon(QMessageBox::Critical);
        errMsg.exec();
        exit(1);
    }
#endif

    asThread = new AsThread(as);

    connect(asThread,SIGNAL(finished()),SLOT(opFinished()));

    loadingDialog = new QDialog(this);
    loadingBar = new QProgressBar(loadingDialog);
    loadingDialog->setLayout(new QVBoxLayout());
    loadingDialog->layout()->addWidget(loadingBar);
    loadingBar->setFormat(tr("Loading %p%"));

    flags = as_QUERY_ALL_INFO | as_EXPERT_QUERY;

    timer = new QTimer();
    timer2 = new QTimer();
    timerUpdate = new QTimer();
    connect(timerUpdate, SIGNAL(timeout()),SLOT(refresh()));
    timer->setSingleShot(true);
    connect(timer,SIGNAL(timeout()),SLOT(addRows()));
    timer->start(100);
    connect(timer2,SIGNAL(timeout()),SLOT(timeFilter()));

    //ui->centralWidget->setDisabled(true);
    ui->mainToolBar->setDisabled(true);

    connect(ui->mainToolBar->addAction(style()->standardIcon(QStyle::SP_BrowserReload), tr("Update")), SIGNAL(triggered()), SLOT(updateDB()));
    markAction = ui->mainToolBar->addAction(style()->standardIcon(QStyle::SP_ArrowUp), tr("Mark all upgrades"));
    connect(markAction,SIGNAL(triggered()),SLOT(markUpgrades()));
    applyAction = ui->mainToolBar->addAction(style()->standardIcon(QStyle::SP_DialogApplyButton), tr("Check and apply"));
    connect(applyAction,SIGNAL(triggered()),SLOT(confirm()));
    applyAction->setDisabled(true);

    connect(ui->searchBar, SIGNAL(textChanged(QString)), SLOT(timerFired(QString)));

    connect(ui->showUpgradable,SIGNAL(toggled(bool)),SLOT(showUpgradable(bool)));
    connect(ui->showInstalled,SIGNAL(toggled(bool)),SLOT(showInstalled(bool)));
    connect(ui->showNotInstalled,SIGNAL(toggled(bool)),SLOT(showNotInstalled(bool)));
    connect(ui->showAll,SIGNAL(toggled(bool)),SLOT(showAll(bool)));
    connect(ui->tableWidget,SIGNAL(cellClicked(int,int)),SLOT(changeStatus(int,int)));
    connect(ui->expertMode,SIGNAL(toggled(bool)),SLOT(expertMode(bool)));

    connect(ui->comboBox,SIGNAL(currentIndexChanged(int)),SLOT(searchTermChanged(int)));

    connect(ui->editCancel,SIGNAL(clicked()),SLOT(editCancel()));
    connect(ui->editConfirm,SIGNAL(clicked()),SLOT(editConfirm()));

    modified=0;

    category = QRegExp(".*");
    category.setCaseSensitivity(Qt::CaseInsensitive);
    category_exclude = QRegExp("---");
    category_exclude.setCaseSensitivity(Qt::CaseInsensitive);
    expert = QRegExp("lib[s]*.*|.*lib[s]*|.*-lib[s]*.*|.*lib[s]*-.*|ttf-.*|.*-data");
    expert.setCaseSensitivity(Qt::CaseInsensitive);

    ui->tableWidget->hideColumn(5);
    ui->tableWidget->hideColumn(6);

    ui->webView->installEventFilter(new QTEventFilter);

    connect(ui->actionAbout_Qt,SIGNAL(triggered()),SLOT(aboutQt()));
    connect(ui->actionAbout_AppSet,SIGNAL(triggered()),SLOT(about()));

    isExpert = false;

    loadingMovie = new QMovie(":/pkgstatus/loading.gif");
    loadingMovie->start();
}

void MainWindow::aboutQt(){
    QMessageBox::aboutQt(this);
}

void MainWindow::about(){
    QMessageBox::about(this, tr("About AppSet-Qt"), tr("An advanced, distribution independent, \"SIMPLE\" and not hungry for dependencies package manager\n\nAuthor: Simone Tobia"));
}

class StatusBarUpdater:public AS::EngineListener{
    QStatusBar *bar;
    QString pre;
    int stepping;
    int i;
    QProgressBar *lb;
public:
    StatusBarUpdater(QStatusBar *bar){this->bar=bar;i=0;stepping=3333;pre=QString("PARSING DATABASE: ");}
    void step(const char *content){
        if(!(i%stepping)){
            QCoreApplication::processEvents(QEventLoop::AllEvents, 33);
            bar->showMessage(pre+content);
            lb->setValue(i*3/9500);
        }        

        i++;
    }

    void setStepping(int s){this->stepping=s;}
    void setPreMessage(QString s){this->pre=s;}
    void setPB(QProgressBar *lb){this->lb=lb;}
};

void MainWindow::refresh(){
    int rows=0;
    switch(asThread->getOp()){
    case 1:
        rows=ui->tableInstall->rowCount();
        for(int i=0;i<rows;++i){
            if(completed[i+baseIndex]) continue;

            Package pkg;
            pkg.setName(ui->tableInstall->item(i,1)->text().toAscii().data());
            float reached = as->getProgressSize(&pkg);
            float total = ui->tableInstall->item(i,4)->text().toFloat()*1024;

            if(ui->tableInstall->item(i,3) && ui->tableInstall->item(i,3)->text().length()){
                QString deps = ui->tableInstall->item(i,3)->text();
                QStringList depsList = deps.split(' ');

                for(QStringList::iterator it2=depsList.begin();it2!=depsList.end();it2++){
                    pkg.setName((*it2).toAscii().data());
                    reached+=as->getProgressSize(&pkg);
                }
            }

            float speed=0;
            if(reached){
                if(baseSizes[i]!=-1){
                    speed=(reached-baseSizes[i])/((float)0.5);
                    ((QProgressBar*)ui->tableInstall->cellWidget(i,5))->setFormat(QString("%p% (")+QString::number((int)speed)+" KB/s)");
                }
                baseSizes[i]=reached;

            }

            float perc = (reached/(float)total)*100;
            ((QProgressBar*)ui->tableInstall->cellWidget(i,5))->setValue((int)perc);

            if(perc>0) ui->tableInstall->scrollToItem(ui->tableInstall->item(i,0));

            if(perc>=99){
                ((QProgressBar*)ui->tableInstall->cellWidget(i,5))->setValue(100);
                ((QProgressBar*)ui->tableInstall->cellWidget(i,5))->setFormat("%p%");
                ui->tableInstall->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/working.png"),"Working"));

                bool totalCompleted=true;
                for(int k=0;totalCompleted && k<rows;++k){
                    if(!completed[k]) totalCompleted=false;
                }

                if(totalCompleted){
                    for(int k=0;k<rows;++k){
                        QLabel *label = new QLabel();
                        label->setMovie(loadingMovie);
                        ui->tableInstall->setCellWidget(k,0,label);
                        ((QProgressBar*)ui->tableInstall->cellWidget(k,5))->setFormat("Installing...");
                    }

                    break;
                }
            }
        }
        break;
     case 2:
        rows=ui->tableUpgraded->rowCount();
        for(int i=0;i<rows;++i){
            if(completed[i+baseIndex]) continue;

            Package pkg;
            pkg.setName(ui->tableUpgraded->item(i,1)->text().toAscii().data());
            float reached = as->getProgressSize(&pkg);
            float total = ui->tableUpgraded->item(i,4)->text().toFloat()*1024;

            float speed=0;
            if(reached){
                if(baseSizes[i+baseIndex]!=-1){
                    speed=(reached-baseSizes[i+baseIndex])/((float)0.5);
                    ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setFormat(QString("%p% (")+QString::number((int)speed)+" KB/s)");
                }
                baseSizes[i+baseIndex]=reached;

            }

            float perc = (reached/(float)total)*100;
            ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setValue((int)perc);

            if(perc>0) ui->tableUpgraded->scrollToItem(ui->tableUpgraded->item(i,0));

            if(perc>=99){
                completed[i+baseIndex]=true;
                ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setValue(100);
                ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setFormat("Waiting others...");
                ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/working.png"),"Working"));


                bool totalCompleted=true;
                for(int k=0;totalCompleted && k<rows;++k){
                    if(!completed[k+baseIndex]) totalCompleted=false;
                }

                if(totalCompleted){
                    for(int k=0;k<rows;++k){
                        QLabel *label = new QLabel();
                        label->setMovie(loadingMovie);
                        ui->tableUpgraded->setCellWidget(k,0,label);
                        ((QProgressBar*)ui->tableUpgraded->cellWidget(k,5))->setFormat("Installing...");
                    }

                    break;
                }
            }
        }
        break;
    }
}

void MainWindow::opFinished(){
    int op = asThread->getOp();
    int status = asThread->getStatus();
    std::list<Package*> *l = asThread->getList();

    switch(op){
    case 1:
        baseIndex = toI;
        toI=0;
        statusI=status;

        if(!statusI){
            int j=0;
            for(std::list<Package*>::iterator it=l->begin();it!=l->end();it++){
                ui->tableInstall->setItem(j++,0,new QTableWidgetItem(QIcon(":pkgstatus/success.png"),"Success"));
                delete (*it);
            }
            delete l;
        }

        break;
    case 2:
        toU=0;
        statusU=status;

        if(!statusU){
            int j=0;
            for(std::list<Package*>::iterator it=l->begin();it!=l->end();it++){
                ui->tableUpgraded->setItem(j++,0,new QTableWidgetItem(QIcon(":pkgstatus/success.png"),"Success"));
                delete (*it);
            }
            delete l;
        }
        break;
    case 3:
        toR=0;
        statusR=status;

        if(!statusR){
            int j=0;
            for(std::list<Package*>::iterator it=l->begin();it!=l->end();it++){
                ui->tableRemoved->setItem(j++,0,new QTableWidgetItem(QIcon(":pkgstatus/success.png"),"Success"));
                delete (*it);
            }
            delete l;
        }
        break;
    }

    timerUpdate->stop();

    if(op>0&&op<4){
        if(!toI && !toU && !toR){
            QMessageBox done;
            status = statusI+statusR+statusU;
            done.setText(status?tr("Errors!"):tr("Success!"));
            done.setInformativeText(status?tr("Some errors during operations, check your network, check your mirrors or try to update..."):tr("All operations completed successfully!"));
            done.setIcon(status?QMessageBox::Critical:QMessageBox::Information);
            if(status)done.exec();
            else done.show();

            ui->stacked->setCurrentIndex(0);
            QCoreApplication::processEvents(QEventLoop::AllEvents,500);
            ui->mainToolBar->show();

            ui->editCancel->setEnabled(true);
            ui->editConfirm->setEnabled(true);

            ui->searchBar->clear();

            ui->tableInstall->show();
            ui->tableUpgraded->show();
            ui->tableRemoved->show();

            modified=0;
            applyEnabler();
            ui->showAll->setChecked(true);
            addRows();
        }else editConfirm();
    }
}

void MainWindow::editConfirm(){    
    ui->editCancel->setDisabled(true);
    ui->editConfirm->setDisabled(true);
    Package *p;

    if(toR){
        std::list<Package*> *prem=new std::list<Package*>();
        for(int i=0;i<toR;++i){
            p=new Package();
            p->setName(ui->tableRemoved->item(i,1)->text().trimmed().toAscii().data());
            prem->insert(prem->end(), p);
            ui->tableRemoved->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/waiting.png"),"Waiting"));
        }

        ui->tableInstall->hide();
        ui->tableUpgraded->hide();
        ui->tableRemoved->show();

        asThread->setList(prem);
        asThread->setOp(3);
        pkgs=prem;
        asThread->start();
    }else if(toI){
        std::list<Package*> *pinst=new std::list<Package*>();
        for(int i=0;i<toI;++i){
            p=new Package();
            p->setName(ui->tableInstall->item(i,1)->text().trimmed().toAscii().data());
            p->setSize(ui->tableInstall->item(i,0)->text().trimmed().toInt());
            p->setRemoteVersion(ui->tableInstall->item(i,2)->text().trimmed().toAscii().data());
            pinst->insert(pinst->end(), p);
            ui->tableInstall->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/waiting.png"),"Waiting"));
        }

        ui->tableInstall->show();
        ui->tableUpgraded->hide();
        ui->tableRemoved->hide();

        asThread->setList(pinst);
        asThread->setOp(1);
        pkgs=pinst;
        asThread->start();
    }else if(toU){
        std::list<Package*> *pupgr=new std::list<Package*>();
        int rows = ui->tableWidget->rowCount();
        for(int i=0;i<rows;++i){
            if(ui->tableWidget->item(i,0)->text()=="Upgradable"){
                p=new Package();
                p->setName(ui->tableWidget->item(i,1)->text().trimmed().toAscii().data());
                pupgr->insert(pupgr->end(),p);
            }
        }
        for(int i=0;i<toU;++i){
            ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/waiting.png"),"Waiting"));
        }

        ui->tableInstall->hide();
        ui->tableUpgraded->show();
        ui->tableRemoved->hide();

        asThread->setList(pupgr);
        asThread->setOp(2);
        pkgs=pupgr;
        asThread->start();
    }

    if(toU || toR || toI) timerUpdate->start(500);
}

void MainWindow::timeFilter(){
    asyncFilter(ui->searchBar->text());
}

void MainWindow::timerFired(QString s){    
    timer2->stop();
    timer2->setSingleShot(true);
    timer2->start(500);s="";
}


void MainWindow::editCancel(){
    ui->stacked->setCurrentIndex(0);
    ui->mainToolBar->show();
}

void MainWindow::markUpgrades(){
    int rows = ui->tableWidget->rowCount();
    for(int i=0;i<rows;++i){
        if(ui->tableWidget->item(i,0)->text()=="Upgradable"){
            currentPacket=i;
            upgrade();
        }
    }
}

void MainWindow::searchTermChanged(int x){
    asyncFilter(ui->searchBar->text());x=0;
}

void MainWindow::asyncFilter(QString filter){
    timer2->stop();
    int rows = ui->tableWidget->rowCount();

    if(filter=="@") filter = ui->searchBar->text();
    else if(filter=="@@"){ filter = ui->searchBar->text(); filterPressed(); return;}
    else filterPressed();

    filter=filter.trimmed();
    filter.replace(' ','|');

    int index = ui->comboBox->currentIndex();

    if(index==2){
        for(int i=0;i<rows;++i){
            if(!ui->tableWidget->item(i,1) || !ui->tableWidget->item(i,4) ||
               (ui->tableWidget->item(i,1)->text().contains(category_exclude) || ui->tableWidget->item(i,4)->text().contains(category_exclude))
                || (!ui->tableWidget->item(i,1)->text().contains(category) && !ui->tableWidget->item(i,4)->text().contains(category))
                || (!ui->tableWidget->item(i,1)->text().contains(QRegExp(filter,Qt::CaseInsensitive)) && !ui->tableWidget->item(i,4)->text().contains(QRegExp(filter,Qt::CaseInsensitive)))){
                ui->tableWidget->hideRow(i);
            }
        }
    }else{
        index=index?4:1;

        for(int i=0;i<rows;++i){
            if(!ui->tableWidget->item(i,1) || !ui->tableWidget->item(i,4) ||
               (ui->tableWidget->item(i,1)->text().contains(category_exclude) || ui->tableWidget->item(i,4)->text().contains(category_exclude)) ||
                (!ui->tableWidget->item(i,1)->text().contains(category) && !ui->tableWidget->item(i,4)->text().contains(category))
                || !ui->tableWidget->item(i,index)->text().contains(QRegExp(filter,Qt::CaseInsensitive))){
                ui->tableWidget->hideRow(i);
            }
        }
    }

    if(!isExpert){
        for(int i=0;i<rows;++i){
            if(ui->tableWidget->item(i,1)->text().contains(expert))
                ui->tableWidget->hideRow(i);
        }
    }

    ui->tableWidget->sortByColumn(1,Qt::AscendingOrder);

    int first=-1;
    for(int i=0;first==-1 && i<rows;++i){
        if(first==-1 && !(ui->tableWidget->isRowHidden(i))){
            first=i;
        }
    }

    if(first!=-1){
        changeStatus(first,4);
        ui->tableWidget->selectRow(first);
    }
}

void MainWindow::filterPressed(){
    if(ui->showAll->isChecked()) showAll(true);
    else if(ui->showInstalled->isChecked()) showInstalled(true);
    else if(ui->showNotInstalled->isChecked()) showNotInstalled(true);
    else if(ui->showUpgradable->isChecked()) showUpgradable(true);
}

void MainWindow::applyEnabler(){
    applyAction->setEnabled(modified>0);
}

void MainWindow::install(){
    ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(QIcon(":pkgstatus/install.png"),"Install"));
    modified++;
    applyEnabler();    
}

void MainWindow::remove(){
    Package p;
    QStringList req;
    p.setName(ui->tableWidget->item(currentPacket,1)->text().trimmed().toAscii().data());
    if((pkgs = as->checkDeps(&p,false))){
        for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
            QString name = QString((*it)->getName().c_str()).trimmed();
            if(name!=QString(p.getName().c_str()))
                req << name;
            delete(*it);
        }
        delete pkgs;
    }
    int res = QMessageBox::Yes;
    if(req.size()){
        QMessageBox reqMes;
        reqMes.setText(tr("Some installed packages require this:"));
        reqMes.setInformativeText(req.join("\n")+tr("\n\nDo you want to proceed anyway?"));
        reqMes.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        res = reqMes.exec();
    }

    if(res == QMessageBox::Yes){
        ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(QIcon(":pkgstatus/remove.png"),"Remove"));
        modified++;
        applyEnabler();
    }
}

void MainWindow::upgrade(){
    ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(style()->standardIcon(QStyle::SP_ArrowUp),"Upgrade"));
    modified++;
    applyEnabler();
}

void MainWindow::notInstall(){
    ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(QIcon(":pkgstatus/unchecked.png"),"Remote"));
    modified--;
    applyEnabler();
}

void MainWindow::notRemove(){
    ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(QIcon(":pkgstatus/checked.png"),"Installed"));
    modified--;
    applyEnabler();
}

void MainWindow::notUpgrade(){
    ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(style()->standardIcon(QStyle::SP_ArrowUp),"Upgradable"));
    modified--;
    applyEnabler();
}

void MainWindow::confirm(){
    ui->stacked->setCurrentIndex(1);
    ui->mainToolBar->hide();

    ui->tableInstall->clearContents();
    ui->tableRemoved->clearContents();
    ui->tableUpgraded->clearContents();
    ui->tableInstall->setRowCount(0);
    ui->tableRemoved->setRowCount(0);
    ui->tableUpgraded->setRowCount(0);

    int in=0,r=0,u=0;

    int rows = ui->tableWidget->rowCount();
    for(int i=0;i<rows;++i){

        if(!isExpert && ui->tableWidget->item(i,0)->text()=="Upgradable" && ui->tableWidget->item(i,1)->text().contains(expert))
            ui->tableWidget->setItem(i,0,new QTableWidgetItem(style()->standardIcon(QStyle::SP_ArrowUp),"Upgrade"));

        if(ui->tableWidget->item(i,0)->text()=="Upgrade"){
            ui->tableUpgraded->insertRow(u);            
            ui->tableUpgraded->setItem(u,0,new QTableWidgetItem(*ui->tableWidget->item(i,0)));
            ui->tableUpgraded->setItem(u,1,new QTableWidgetItem(*ui->tableWidget->item(i,1)));
            ui->tableUpgraded->setItem(u,2,new QTableWidgetItem(*ui->tableWidget->item(i,2)));
            ui->tableUpgraded->setItem(u,3,new QTableWidgetItem(*ui->tableWidget->item(i,3)));

           /* Package p;
            p.setName(ui->tableUpgraded->item(u,1)->text().trimmed().toAscii().data());
            QStringList deps;
            if(pkgs = as->checkDeps(&p,true,true)){
                for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                    QString name = QString((*it)->getName().c_str()).trimmed();
                    if(name!=QString(p.getName().c_str()))
                        deps << name;
                    delete(*it);
                }
                delete pkgs;
            }

            ui->tableUpgraded->setItem(u,4,new QTableWidgetItem(deps.join(" ")));*/

            QProgressBar *prog=new QProgressBar();
            prog->setValue(0);
            //prog->setMaximumWidth(200);
            ui->tableUpgraded->setCellWidget(u,5,prog);

            //ui->tableUpgraded->setItem(u++,4,new QTableWidgetItem(*ui->tableWidget->item(i,4)));
            ui->tableUpgraded->setItem(u++,4,new QTableWidgetItem(QString::number(ui->tableWidget->item(i,6)->text().toFloat()/1024)));

//            ui->tableUpgraded->showRow(u++);
        }else if(ui->tableWidget->item(i,0)->text()=="Install"){
            ui->tableInstall->insertRow(in);
            ui->tableInstall->setItem(in,0,new QTableWidgetItem(*ui->tableWidget->item(i,0)));
            ui->tableInstall->item(in,0)->setText(ui->tableWidget->item(i,6)->text());
            ui->tableInstall->setItem(in,1,new QTableWidgetItem(*ui->tableWidget->item(i,1)));
            ui->tableInstall->setItem(in,2,new QTableWidgetItem(*ui->tableWidget->item(i,3)));

            Package p;
            int dsize = ui->tableWidget->item(i,6)->text().toFloat();
            p.setName(ui->tableInstall->item(in,1)->text().trimmed().toAscii().data());
            QStringList deps;
            if((pkgs = as->checkDeps(&p,true))){
                for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                    QString name = QString((*it)->getName().c_str()).trimmed();
                    if(name!=QString(p.getName().c_str())){
                        deps << name;
                        dsize+=QString((*it)->getRemoteVersion().c_str()).toFloat()*1024;
                    }
                    delete(*it);
                }
                delete pkgs;
            }

            ui->tableInstall->setItem(in,3,new QTableWidgetItem(deps.join(" ")));


            QProgressBar *prog=new QProgressBar();
            prog->setValue(0);
            //prog->setMaximumWidth(200);
            ui->tableInstall->setCellWidget(in,5,prog);

            //ui->tableInstall->setItem(in++,4,new QTableWidgetItem(*ui->tableWidget->item(i,4)));
            ui->tableInstall->setItem(in++,4,new QTableWidgetItem(QString::number(dsize/(float)1024)));
        }else if(ui->tableWidget->item(i,0)->text()=="Remove"){
            ui->tableRemoved->insertRow(r);
            ui->tableRemoved->setItem(r,0,new QTableWidgetItem(*ui->tableWidget->item(i,0)));
            ui->tableRemoved->setItem(r,1,new QTableWidgetItem(*ui->tableWidget->item(i,1)));

            Package p;
            p.setName(ui->tableRemoved->item(r,1)->text().trimmed().toAscii().data());
            QStringList deps;
            if((pkgs = as->checkDeps(&p,false))){
                for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                    QString name = QString((*it)->getName().c_str()).trimmed();
                    if(name!=QString(p.getName().c_str()))
                        deps << name;
                    delete(*it);
                }
                delete pkgs;
            }

            ui->tableRemoved->setItem(r,2,new QTableWidgetItem(deps.join(" ")));

            ui->tableRemoved->setItem(r++,3,new QTableWidgetItem(*ui->tableWidget->item(i,4)));
        }
    }
    if(!in){
        ui->tableInstall->hide();
        ui->lblInstalled->hide();
    }else{
        ui->tableInstall->show();
        ui->lblInstalled->show();
    }

    if(!u){
        ui->tableUpgraded->hide();
        ui->lblUpgraded->hide();
    }else{
        ui->tableUpgraded->show();
        ui->lblUpgraded->show();
    }

    if(!r){
        ui->tableRemoved->hide();
        ui->lblRemoved->hide();
    }else{
        ui->tableRemoved->show();
        ui->lblRemoved->show();
    }

    toI=in;toU=u;toR=r;
    statusI=statusU=statusR=0;
    baseIndex=0;
    baseSizes.reserve(in+u);
    completed.reserve(in+u);
    for(int k=0;k<in+u;++k){
        baseSizes[k]=-1;
        completed[k]=false;
    }
}

void MainWindow::changeStatus(int row, int col){    
    if(ui->tableWidget->item(row,5) && ui->tableWidget->item(row,1)){
        if(ui->webView && ui->webView->isEnabled()){
            ui->webView->stop();
            ui->webView->setUrl(QUrl::fromUserInput(ui->tableWidget->item(row,5)->text().trimmed()));
        }
        Package p;
        p.setName(ui->tableWidget->item(row,1)->text().trimmed().toAscii().data());
        ui->infoText->clear();

        ui->infoText->append(QString(tr("<b>Size: </b>"))+ui->tableWidget->item(row,6)->text()+" KB");

        ui->infoText->append(QString(tr("<b>URL: </b>"))+ui->tableWidget->item(row,5)->text());
        ui->infoText->append(QString("<a href=\"")+ui->tableWidget->item(row,5)->text()+QString("\" >")+QString(tr("(Watch the full site)"))+QString("</a>"));

        ui->infoText->append(tr("<b>Requires:</b>"));
        if((pkgs = as->checkDeps(&p,true))){
            for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                QString name = QString((*it)->getName().c_str()).trimmed();
                if(name!=QString(p.getName().c_str())){
                    ui->infoText->append(name+QString(" (")+QString((*it)->getRemoteVersion().c_str())+QString(" MB)"));
                }
                delete(*it);
            }
            delete pkgs;
        }

        ui->infoText->append(tr("<b>Required By:</b>"));
        if((pkgs = as->checkDeps(&p,false))){
            for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                QString name = QString((*it)->getName().c_str()).trimmed();
                if(name!=QString(p.getName().c_str())){
                    ui->infoText->append(name);
                }
                delete(*it);
            }
            delete pkgs;
        }
    }

    if(col==0){
        QMenu menu(this);
        QAction install(QIcon(":pkgstatus/install.png"),tr("Install"),this);
        QAction remove(QIcon(":pkgstatus/remove.png"),tr("Remove"),this);
        QAction upgrade(style()->standardIcon(QStyle::SP_ArrowUp),tr("Upgrade"),this);
        QAction cancel(style()->standardIcon(QStyle::SP_DialogDiscardButton),tr("Cancel"),this);
        QList<QAction*>actions;

        currentPacket = row;

        connect(&install, SIGNAL(triggered()),SLOT(install()));
        connect(&remove, SIGNAL(triggered()),SLOT(remove()));
        connect(&upgrade, SIGNAL(triggered()),SLOT(upgrade()));

        QString text = ui->tableWidget->item(row,0)?ui->tableWidget->item(row,0)->text():0;

        if(text=="Install" || text=="Upgrade" || text=="Remove") actions.append(&cancel);
        else{
            actions.append(&install);
            actions.append(&upgrade);
            actions.append(&remove);
        }

        menu.addActions(actions);

        if(text=="Remote"){
            remove.setDisabled(true);
            upgrade.setDisabled(true);
        }else if(text=="Upgradable"){
            install.setDisabled(true);
        }else if(text=="Installed"){
            install.setDisabled(true);
            upgrade.setDisabled(true);
        }else if(text=="Remove"){
            connect(&cancel,SIGNAL(triggered()),SLOT(notRemove()));
        }else if(text=="Install"){
            connect(&cancel,SIGNAL(triggered()),SLOT(notInstall()));
        }else if(text=="Upgrade"){
            connect(&cancel,SIGNAL(triggered()),SLOT(notUpgrade()));
        }

        menu.exec(QCursor::pos());

        disconnect(&install, SIGNAL(triggered()),this,SLOT(install()));
        disconnect(&remove, SIGNAL(triggered()),this,SLOT(remove()));
        disconnect(&upgrade, SIGNAL(triggered()),this,SLOT(upgrade()));
        disconnect(&cancel, SIGNAL(triggered()),0,0);
    }
}

void MainWindow::updateDB(){
    ui->centralWidget->setDisabled(true);
    ui->mainToolBar->setDisabled(true);    

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    StatusBarUpdater *sbu = new StatusBarUpdater(ui->statusBar);
    sbu->setPB(loadingBar);
    //loadingDialog->show();
    sbu->setStepping(1);
    sbu->setPreMessage(tr("UPDATING DB: "));


    ui->statusBar->showMessage(tr("UPDATING DB..."),5000);

    as->addListener(sbu);
    as->update();
    as->removeListener(sbu);

    //ui->showUpgradable->setChecked(true);

    ui->statusBar->showMessage(tr("DB UPDATED!"),5000);

    //loadingDialog->hide();
    modified=0;
    applyEnabler();


    addRows(true);
}

void MainWindow::expertMode(bool mode){
    this->isExpert = mode;

    asyncFilter("@@");
}

int MainWindow::visibleRowCount(){
    int rows=ui->tableWidget->rowCount();
    int ret=rows;

    for(int i=0;i<rows;++i) if(ui->tableWidget->isRowHidden(i)) ret--;

    return ret;
}

void MainWindow::showGames(){
    category.setPattern("rpg|fps|game|games|race|racing|funny|shooter");
    category_exclude.setPattern("configuration[s]*|kernel|driver[s]*|firmware[s]*|libraries|library|protocol[s]*|dns|debug|traceroute|povray|screenshooter");
    ui->tabWidget->setCurrentIndex(1);
    ui->tabWidget->setTabIcon(1, QIcon(":/pkggroups/games.png"));
    asyncFilter("@@");    
    ui->tabWidget->setTabText(1, tr("Games")+QString(" (")+QString::number(visibleRowCount())+QString(")"));
}

void MainWindow::showSystem(){
    category.setPattern("system|configuration[s]*|kernel|driver[s]*|firmware[s]*|libraries|library|protocol[s]*");
    category_exclude.setPattern("game[s]*|racing");

    ui->tabWidget->setCurrentIndex(1);    
    ui->tabWidget->setTabIcon(1, QIcon(":/pkggroups/system.png"));

    asyncFilter("@@");

    ui->tabWidget->setTabText(1, tr("System")+QString(" (")+QString::number(visibleRowCount())+QString(")"));
}

void MainWindow::showMultimedia(){
    category.setPattern("audio|video|player|codec[s]*");
    category_exclude.setPattern("configuration[s]*|kernel|driver[s]*|firmware[s]*|libraries|library|protocol[s]*|rpg|fps|game|games|racing");

    ui->tabWidget->setCurrentIndex(1);
    ui->tabWidget->setTabIcon(1, QIcon(":/pkggroups/multimedia.png"));

    asyncFilter("@@");

    ui->tabWidget->setTabText(1, tr("Multimedia")+QString(" (")+QString::number(visibleRowCount())+QString(")"));
}

void MainWindow::showOffice(){
    category.setPattern(".*office|spreadsheed|word processor");
    category_exclude.setPattern("configuration[s]*|kernel|driver[s]*|firmware[s]*|libraries|library|protocol[s]*");

    ui->tabWidget->setCurrentIndex(1);
    ui->tabWidget->setTabIcon(1, QIcon(":/pkggroups/office.png"));

    asyncFilter("@@");

    ui->tabWidget->setTabText(1, tr("Office")+QString(" (")+QString::number(visibleRowCount())+QString(")"));
}

void MainWindow::showInternet(){
    category.setPattern("email|mail|browser|mozilla|google|opera");
    category_exclude.setPattern("gimp|image|configuration[s]*|kernel|driver[s]*|firmware[s]*|libraries|library|protocol[s]*");

    ui->tabWidget->setCurrentIndex(1);
    ui->tabWidget->setTabIcon(1, QIcon(":/pkggroups/internet.png"));

    asyncFilter("@@");

    ui->tabWidget->setTabText(1, tr("Internet")+QString(" (")+QString::number(visibleRowCount())+QString(")"));
}

void MainWindow::showUpgradable(bool checked){
    if(!checked) return;

    int rows = ui->tableWidget->rowCount();

    for(int i=0;i<rows;++i){
        if(ui->tableWidget->item(i,0)->text()=="Upgradable" || ui->tableWidget->item(i,0)->text()=="Upgrade"){
            ui->tableWidget->showRow(i);
        }else{
            ui->tableWidget->hideRow(i);
        }
    }

    asyncFilter();
}

void MainWindow::showInstalled(bool checked){
    if(!checked) return;

    int rows = ui->tableWidget->rowCount();

    for(int i=0;i<rows;++i){
        if(ui->tableWidget->item(i,0)->text()!="Installed" && ui->tableWidget->item(i,0)->text()!="Remove" && ui->tableWidget->item(i,0)->text()!="Upgradable" && ui->tableWidget->item(i,0)->text()!="Upgrade"){
            ui->tableWidget->hideRow(i);
        }else{
            ui->tableWidget->showRow(i);
        }
    }

    asyncFilter();
}

void MainWindow::showAll(bool checked){
    if(!checked) return;

    int rows = ui->tableWidget->rowCount();

    for(int i=0;i<rows;++i){
        ui->tableWidget->showRow(i);
    }

    asyncFilter();
}

void MainWindow::showAllCat(){
    category.setPattern(".*");
    category_exclude.setPattern("---");

    ui->tabWidget->setCurrentIndex(1);
    ui->tabWidget->setTabIcon(1, QIcon(":/pkggroups/all.png"));

    if(ui->tabWidget->tabText(1)!="All") asyncFilter("@@");

    ui->tabWidget->setTabText(1, tr("All")+QString(" (")+QString::number(visibleRowCount())+QString(")"));
}

void MainWindow::showNotInstalled(bool checked){
    if(!checked) return;

    int rows = ui->tableWidget->rowCount();

    for(int i=0;i<rows;++i){
        if(ui->tableWidget->item(i,0)->text()!="Install" && ui->tableWidget->item(i,0)->text()!="Remote"){
            ui->tableWidget->hideRow(i);
        }else{
            ui->tableWidget->showRow(i);
        }
    }

    asyncFilter();
}

void MainWindow::addRows(bool checked){
    if(!checked) return;

    ui->centralWidget->setDisabled(true);
    ui->mainToolBar->setDisabled(true);

    //ui->stacked->setCurrentIndex(2);
    loadingDialog->show();
    loadingBar->setValue(0);

    ui->tableWidget->setSortingEnabled(false);

    timer2->stop();

    QCoreApplication::processEvents(QEventLoop::AllEvents, 500);


    //ui->tableWidget->clearContents();
    int count = ui->tableWidget->rowCount();
    for(int i=0;i<count;++i)ui->tableWidget->removeRow(0);
    //ui->tableWidget->setRowCount(0);

    StatusBarUpdater sbu(ui->statusBar);
    sbu.setPB(loadingBar);

    as->addListener(&sbu);
    pkgs = as->queryRemote(flags);
    as->removeListener(&sbu);

    AsThread t2(as);
    t2.setOp(4);
    t2.start();

    //loadingBar->setValue(30);

    QTableWidgetItem *newItem;
    int i=0;
    ui->tableWidget->setRowCount(pkgs->size());
    for(std::list<Package*>::iterator it=pkgs->begin(); it!=pkgs->end(); it++){
        Package *pkg = *it;

        //ui->tableWidget->insertRow(i);
        newItem = new QTableWidgetItem(QIcon(":pkgstatus/unchecked.png"),"Remote");
        newItem->setToolTip(tr("Not Installed"));
        ui->tableWidget->setItem(i,0, newItem);
        newItem = new QTableWidgetItem(pkg->getName().c_str());

        ui->tableWidget->setItem(i,1, newItem);
        newItem = new QTableWidgetItem(pkg->getRemoteVersion().c_str());
        ui->tableWidget->setItem(i,3, newItem);
        newItem = new QTableWidgetItem(pkg->getDescription().c_str());
        ui->tableWidget->setItem(i,4, newItem);
        newItem = new QTableWidgetItem(pkg->getURL().c_str());
        ui->tableWidget->setItem(i,5, newItem);
        newItem = new QTableWidgetItem(QString::number(pkg->getSize()));
        ui->tableWidget->setItem(i,6, newItem);

        i++;

        delete pkg;

        if(!(i%100)){
            loadingBar->setValue(30+i*20/pkgs->size());
            QCoreApplication::processEvents(QEventLoop::AllEvents, 33);
        }
    }

    delete pkgs;

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    sbu.setPreMessage(tr("PARSING INSTALLED: "));
    sbu.setStepping(60);
    //as->addListener(&sbu);
    //pkgs = as->queryLocal(flags);
    //as->removeListener(&sbu);
    t2.wait();
    pkgs=t2.getList();

    //loadingBar->setValue(60);

    ui->statusBar->showMessage(tr("SEARCHING CORRESPONDENCES..."));

    int k=0;
    int rows = ui->tableWidget->rowCount();
    QList<int> toRemove;
    for(std::list<Package*>::iterator it=pkgs->begin(); it!=pkgs->end(); it++){
        Package *pkg = *it;

        int found=-1;
        QTableWidgetItem* versionMatch;
        for(int index=0;index<rows;++index){
            if(ui->tableWidget->item(index,1)->text()==QString(pkg->getName().c_str())){
                if(found!=-1){
                    if((((QTNIXEngine*)as)->compareVersions(ui->tableWidget->item(index,3)->text(),versionMatch->text()))<0){
                        toRemove.insert(toRemove.end(),index);
                    }else{
                        toRemove.insert(toRemove.end(),found);
                        found=index;
                        versionMatch=ui->tableWidget->item(index,3);
                    }
                }else{
                    versionMatch=ui->tableWidget->item(index,3);
                    found=index;
                }
            }
        }

        if(found!=-1){
            newItem = new QTableWidgetItem(QIcon(":pkgstatus/checked.png"),"Installed");
            newItem->setToolTip(tr("Installed"));
            ui->tableWidget->setItem(found,0, newItem);
            newItem = new QTableWidgetItem(pkg->getLocalVersion().c_str());
            ui->tableWidget->setItem(found,2, newItem);
        }else{
            ui->tableWidget->insertRow(i);
            newItem = new QTableWidgetItem(QIcon(":pkgstatus/checked.png"),"Installed");
            newItem->setToolTip(tr("Installed (external)"));
            ui->tableWidget->setItem(i,0, newItem);
            newItem = new QTableWidgetItem(pkg->getName().c_str());
            ui->tableWidget->setItem(i,1, newItem);
            newItem = new QTableWidgetItem(pkg->getLocalVersion().c_str());
            ui->tableWidget->setItem(i,2, newItem);
            newItem = new QTableWidgetItem((QString(" (")+tr("External")+QString(")")));
            ui->tableWidget->setItem(i,3, newItem);
            newItem = new QTableWidgetItem(pkg->getDescription().c_str());
            ui->tableWidget->setItem(i,4, newItem);
            newItem = new QTableWidgetItem(pkg->getURL().c_str());
            ui->tableWidget->setItem(i,5, newItem);
            newItem = new QTableWidgetItem(QString::number(pkg->getSize()));
            ui->tableWidget->setItem(i,6, newItem);
            i++;
            rows++;
        }

        k++;

        if(!(k%11)){
            loadingBar->setValue(50+k*45/pkgs->size());
            QCoreApplication::processEvents(QEventLoop::AllEvents, 33);
        }

        delete pkg;
    }



    delete pkgs;

    QCoreApplication::processEvents(QEventLoop::AllEvents, 33);

    ui->statusBar->showMessage("CHECKING UPGRADABLES", 5000);

    rows = ui->tableWidget->rowCount();
    int upgradables=0;
    for(int i=0;i<rows;++i){
        if(ui->tableWidget->item(i,0) && (ui->tableWidget->item(i,0)->text()=="Installed" || ui->tableWidget->item(i,0)->text()=="Remove")
             && ui->tableWidget->item(i,2) && ui->tableWidget->item(i,3) && ui->tableWidget->item(i,3)->text()!=(QString(" (")+tr("External")+QString(")")) && (((QTNIXEngine*)as)->compareVersions(ui->tableWidget->item(i,2)->text(),ui->tableWidget->item(i,3)->text()))<0){
            newItem = new QTableWidgetItem(QIcon(":pkgstatus/upgrade.png"),"Upgradable");
            newItem->setToolTip(tr("Upgradable"));
            upgradables++;
            ui->tableWidget->setItem(i,0,newItem);
            QCoreApplication::processEvents(QEventLoop::AllEvents, 33);
        }
        loadingBar->setValue(95+i*5/rows);
    }

    markAction->setEnabled(upgradables>0);

    markAction->setText(tr("Mark all upgrades")+QString(" (")+QString::number(upgradables)+QString(")"));

    ui->statusBar->showMessage(QString("INFO UPDATED: ")+QString::number(rows)+QString(" PACKAGES SHOWN"), 10000);


    loadingBar->setValue(100);

    QCoreApplication::processEvents(QEventLoop::AllEvents, 33);

    ui->showAll->setChecked(true);
    ui->tableWidget->setSortingEnabled(true);
    asyncFilter();
    //timer2->start();

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    //ui->stacked->setCurrentIndex(0);
    loadingDialog->hide();
    loadingBar->setValue(0);

    ui->centralWidget->setEnabled(true);
    ui->mainToolBar->setEnabled(true);

    //ui->tableWidget->sortByColumn(1,Qt::AscendingOrder);

    if(ui->tabWidget->tabText(1)=="All")ui->tabWidget->setTabText(1, tr("All")+QString(" (")+QString::number(visibleRowCount())+QString(")"));
}

MainWindow::~MainWindow(){
    delete ui;

#ifdef unix
    delete (AS::QTNIXEngine*)as;
#endif
}
