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

#include  <QHBoxLayout>
#include  <QVBoxLayout>
#include <QPushButton>
#include <QUrl>
#include <QDebug>

#include <QDesktopServices>
#include <QProcess>

#include <QMenu>

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

#include <QSplitter>
#include <QWidgetList>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), currentReply(0){

    ui->setupUi(this);

    merging = true;

    local=false;

    QStringList headers;
    headers << tr("S") << tr("Repository") << tr("Packet") << tr("Installed Version") << tr("Last Version") << tr("Description");
    ui->tableWidget->setColumnWidth(0,24);
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->setColumnWidth(1,100);

    QStringList updateHeaders;
    updateHeaders << tr("S") << tr("Packet") << tr("Version") << tr("Dependencies") << tr("Size(MB)") << tr("Progress");
    ui->tableUpgraded->setColumnWidth(0,24);
    ui->tableUpgraded->setHorizontalHeaderLabels(updateHeaders);

    ui->editConfirm->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    ui->editCancel->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));

#ifdef unix
    as = new AS::QTNIXEngine();
    int errno=0;
    if((errno=((AS::QTNIXEngine*)as)->configure("/etc/appset.conf"))){
        QMessageBox errMsg;
        errMsg.setText(tr("Errors while initializing the system!"));
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
    loadingDialog->setLayout(new QHBoxLayout());
    QLabel *lblLoading = new QLabel(loadingDialog);
    lblLoading->setPixmap(QPixmap(":/general/loading.png"));;
    lblLoading->setScaledContents(true);
    lblLoading->setFixedSize(loadingBar->height()*2,loadingBar->height()*2);
    loadingDialog->layout()->addWidget(lblLoading);
    loadingDialog->layout()->addWidget(loadingBar);

    flags = as_QUERY_ALL_INFO | as_EXPERT_QUERY;

    timer = new QTimer();
    timer2 = new QTimer();
    timerUpdate = new QTimer();
    connect(timerUpdate, SIGNAL(timeout()),SLOT(refresh()));
    timer->setSingleShot(true);
    connect(timer,SIGNAL(timeout()),SLOT(addRows()));
    argsParsed=false;
    timer->start(100);
    connect(timer2,SIGNAL(timeout()),SLOT(timeFilter()));

    timerConfirm = new QTimer();
    connect(timerConfirm,SIGNAL(timeout()),SLOT(confirmTimeout()));

    //ui->centralWidget->setDisabled(true);
    ui->mainToolBar->setDisabled(true);

    connect(ui->mainToolBar->addAction(style()->standardIcon(QStyle::SP_BrowserReload), tr("Update")), SIGNAL(triggered()), SLOT(updateDB()));
#ifdef unix
    cleanAction = ui->mainToolBar->addAction(QIcon(":editing/clear.png"), tr("Clean cache"), this, SLOT(cleanCache()));
    cleanAction->setDisabled(true);    
#endif
    markAction = ui->mainToolBar->addAction(style()->standardIcon(QStyle::SP_ArrowUp), tr("Mark all upgrades"));
    connect(markAction,SIGNAL(triggered()),SLOT(markUpgrades()));

    openLocalAction = ui->mainToolBar->addAction(style()->standardIcon(QStyle::SP_DialogOpenButton),tr("Open local package"));
    connect(openLocalAction,SIGNAL(triggered()),SLOT(openLocal()));

    applyAction = ui->mainToolBar->addAction(style()->standardIcon(QStyle::SP_DialogApplyButton), tr("Check and apply"));
    connect(applyAction,SIGNAL(triggered()),SLOT(confirm()));
    applyAction->setDisabled(true);

    ui->mainToolBar->addSeparator();

    connect(ui->mainToolBar->addAction(style()->standardIcon(QStyle::SP_ComputerIcon), tr("Options")),SIGNAL(triggered()),SLOT(showOptions()));

    ui->mainToolBar->addSeparator();

    connect(ui->mainToolBar->addAction(QIcon(":general/appset.png"), tr("About")),SIGNAL(triggered()),SLOT(about()));

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

    connect(ui->clearButton,SIGNAL(clicked()),ui->searchBar,SLOT(clear()));

    modified=0;

    category = QRegExp(".*");
    category.setCaseSensitivity(Qt::CaseInsensitive);
    category_exclude = QRegExp("---");
    category_exclude.setCaseSensitivity(Qt::CaseInsensitive);
    expert = QRegExp("lib[^r][s]*.*|.*lib[^r][s]*|.*-lib[s]*.*|.*lib[^r][s]*-.*|ttf-.*|.*-data");
    expert.setCaseSensitivity(Qt::CaseInsensitive);

    ui->tableWidget->hideColumn(1);
    ui->tableWidget->hideColumn(6);
    ui->tableWidget->hideColumn(7);

    ui->webView->installEventFilter(new QTEventFilter);

    connect(ui->actionAbout_Qt,SIGNAL(triggered()),SLOT(aboutQt()));
    connect(ui->actionAbout_AppSet,SIGNAL(triggered()),SLOT(about()));

    isExpert = false;

    loadingMovie = new QMovie(":/pkgstatus/loading.gif");
    loadingMovie->start();

    //RSS
    connect(ui->treeWidget, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
            this, SLOT(itemActivated(QTreeWidgetItem*)));    
    QStringList headerLabels;
    headerLabels << tr("Link") << tr("Title");
    ui->treeWidget->setHeaderLabels(headerLabels);
    ui->treeWidget->header()->setResizeMode(QHeaderView::ResizeToContents);
    get(QUrl(QString(((AS::QTNIXEngine*)as)->getNewsUrl(QLocale::languageToString(QLocale::system().language()).toAscii().data()).c_str())));
    ui->treeWidget->hideColumn(0);

    QSplitter *splitter = new QSplitter(ui->tabList);
    ui->contents->removeWidget(ui->extraInfoGroupBox);
    ui->contents->removeWidget(ui->tableWidget);
    ui->tabList->layout()->addWidget(splitter);
    splitter->addWidget(ui->tableWidget);
    splitter->addWidget(ui->extraInfoGroupBox);
    splitter->setOrientation(Qt::Vertical);

    QList<int> sizes;
    sizes << 250 << 180;
    splitter->setSizes(sizes);

    Options opt;
    this->sbdelay=opt.sbdelay;
    this->extbrowser=opt.browser;
    this->showBackOut=opt.backOutput;
    this->confirmCountdown=opt.confirmCountdown;
    if(!opt.statShow) ui->statGroup->setHidden(true);
    if(opt.startfullscreen)this->showMaximized();
    if(opt.showRepos)ui->tableWidget->showColumn(1);
    else ui->tableWidget->hideColumn(1);

    connect(ui->infoText,SIGNAL(anchorClicked(QUrl)),SLOT(extBrowserLink(QUrl)));

    ui->backGroup->setHidden(true);
}

#include <QFileDialog>
void MainWindow::openLocal(QString fileName){
    //XXX More than 1 file

    if(fileName=="")fileName=QFileDialog::getOpenFileName(this,
        tr("Open local package"),
        QDesktopServices::storageLocation(QDesktopServices::HomeLocation),
        tr("Package files")+QString(" (*")+QString(((AS::QTNIXEngine*)as)->getLocalExt().c_str())+QString(")"));

    if(fileName=="") return;

    AS::Package pkg;
    pkg.setName(fileName.toAscii().data());
    std::list<Package*>* pkgs=((AS::QTNIXEngine*)as)->checkDeps(&pkg,true,false,true);

    if(pkgs){
        ui->tableWidget->setRowCount(0);
        ui->tableWidget->clearContents();

        ui->tableWidget->setRowCount(1);
        ui->tableWidget->setItem(0,0,new QTableWidgetItem(QIcon(":pkgstatus/install.png"),"Install"));
        ui->tableWidget->setItem(0,1, new QTableWidgetItem((*(pkgs->rbegin()))->getName().c_str()));
        ui->tableWidget->setItem(0,3, new QTableWidgetItem(fileName));
        ui->tableWidget->setItem(0,6,new QTableWidgetItem(QString((*(pkgs->rbegin()))->getRemoteVersion().c_str())));

        delete pkgs;

        local=true;
        confirm();
    }else{
        QMessageBox error;
        error.setText(tr("Error loading the specified package file!"));
        error.setInformativeText(tr("The file")+QString(" \"")+QString(fileName)+QString("\" ")+tr("doesn't seems to be a valid package!"));
        error.setIcon(QMessageBox::Critical);
        error.setStandardButtons(QMessageBox::Ok);
        error.exec();

        addRows();
    }
}

void MainWindow::extBrowserLink(const QUrl & link){
    if(extbrowser==""){
        QDesktopServices::openUrl(link);
    }else{
        QStringList args;
        args = extbrowser.split(' ');
        QString browserExe = args.at(0);
        args.removeAt(0);
        args << link.toString();
        QProcess::startDetached(browserExe,args);
    }
}

void MainWindow::showOptions(){
    Options opt;
    int res = opt.exec();

    if(res==QDialog::Accepted){//Accepted new options
        //Saving new options
        opt.writeConfigFile();

        //Activating new options
        this->sbdelay=opt.sbdelay;
        this->extbrowser=opt.browser;
        this->showBackOut=opt.backOutput;
        this->confirmCountdown=opt.confirmCountdown;
        if(!opt.statShow) ui->statGroup->setHidden(true);
        else ui->statGroup->setVisible(true);
        if(opt.showRepos)ui->tableWidget->showColumn(1);
        else ui->tableWidget->hideColumn(1);
    }
}

//RSS
void MainWindow::get(const QUrl &url){
    QNetworkRequest request(url);
    if (currentReply) {
        currentReply->disconnect(this);
        currentReply->deleteLater();
    }
    currentReply = manager.get(request);
    connect(currentReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(currentReply, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
    connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
}

void MainWindow::finished(QNetworkReply *reply){
    Q_UNUSED(reply);
}

#include <QDesktopServices>
void MainWindow::itemActivated(QTreeWidgetItem * item){
    ui->textBrowser->setHtml(item->text(0));
}

void MainWindow::metaDataChanged(){
    QUrl redirectionTarget = currentReply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectionTarget.isValid()) {
        get(redirectionTarget);
    }
}

void MainWindow::readyRead(){
    int statusCode = currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode >= 200 && statusCode < 300) {
        QByteArray data = currentReply->readAll();
        xml.addData(data);
        parseXml();
    }
}

void MainWindow::parseXml(){
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "item")
                linkString = xml.attributes().value("rss:about").toString();
            currentTag = xml.name().toString();
        } else if (xml.isEndElement()) {
            if (xml.name() == "item") {

                QTreeWidgetItem *item = new QTreeWidgetItem;
                item->setText(1, titleString);
                item->setText(0, descString);
                ui->treeWidget->addTopLevelItem(item);
                if(ui->treeWidget->topLevelItemCount()==1){
                    ui->treeWidget->setCurrentItem(item);
                    itemActivated(item);
                }
                //ui->textBrowser->setHtml(descString);

                titleString.clear();
                linkString.clear();
                descString.clear();
            }

        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            if (currentTag == "title")
                titleString = xml.text().toString();
            else if (currentTag == "link")
                linkString += xml.text().toString();
            else if (currentTag == "description")
                descString += xml.text().toString();
        }
    }
    if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
    }
}
void MainWindow::error(QNetworkReply::NetworkError){
    qWarning("error retrieving RSS feed");
    currentReply->disconnect(this);
    currentReply->deleteLater();
    currentReply = 0;
    ui->newsGroup->hide();
}

//END RSS

void MainWindow::cleanCache(){
#ifdef unix
    ((AS::QTNIXEngine*)as)->cleanCache();
    cleanAction->setDisabled(true);
    cleanAction->setText(tr("Clean cache"));
#endif
}

void MainWindow::confirmTimeout(){
    confirmRemaining--;

    if(confirmRemaining==0){
        timerConfirm->stop();

        ui->editConfirm->setText(tr("Confirm"));

        editConfirm();
    }else ui->editConfirm->setText(tr("Confirm")+QString(" (")+QString::number(confirmRemaining)+QString(")"));
}

void MainWindow::aboutQt(){
    QMessageBox::aboutQt(this);
}

void MainWindow::about(){
    QMessageBox::about(this, tr("About AppSet-Qt"), tr("An advanced and feature rich Package Manager Frontend\n\nAuthor: Simone Tobia")+
                       QString("\n\n")+tr("A special thanks goes to the Chakra-project team for their suggestions and translations."));
}

#include <QFile>
void MainWindow::refresh(){
    QStringList logs;
    QFile logFile;
    QByteArray preLog;

#ifdef unix
    if(QFile::exists("/var/log/appset.log")){
        logFile.setFileName("/var/log/appset.log");
#endif
        logFile.open(QFile::ReadOnly);
        while(!logFile.atEnd()){
            preLog = logFile.readLine();
            if(preLog.size()) logs.append(preLog);
        }

        if(logs.size()) ui->backOut->setText(logs.last());
    }


    int rows=0;
    switch(asThread->getOp()){
    case 1: //Install
        rows=ui->tableUpgraded->rowCount();
        for(int i=0;i<rows;++i){
            if(ui->tableUpgraded->item(i,0)->text()=="Upgrade" || ui->tableUpgraded->item(i,0)->text()=="Remove" || completed[i+baseIndex]) continue;

            Package pkg;
            pkg.setName(ui->tableUpgraded->item(i,1)->text().split('/').at(1).toAscii().data());

            QString remoteversion = ui->tableUpgraded->item(i,2)->text();
            pkg.setRemoteVersion(remoteversion.toAscii().data());

            float reached = as->getProgressSize(&pkg);
            float total = ui->tableUpgraded->item(i,4)->text().toFloat()*1024;

            if(ui->tableUpgraded->item(i,3) && ui->tableUpgraded->item(i,3)->text().length()){
                QString deps = ui->tableUpgraded->item(i,3)->text();
                QStringList depsList = deps.split(' ');

                for(QStringList::iterator it2=depsList.begin();it2!=depsList.end();it2++){
                    pkg.setName((*it2).split('/').at(1).toAscii().data());
                    reached+=as->getProgressSize(&pkg, true);
                }
            }

            float speed=0;
            if(reached){
                if(baseSizes[i]!=-1){
                    speed=(reached-baseSizes[i])/((float)0.25);
                    ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setFormat(QString("%p% (")+QString::number((int)speed)+" KB/s)");
                }
                baseSizes[i]=reached;

            }

            float perc = total?(reached/(float)total)*100:100;
            ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setValue((int)perc);

            if(perc>0) ui->tableUpgraded->scrollToItem(ui->tableUpgraded->item(i,0));

            if(perc>=99){
                ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setValue(100);
                ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setFormat(tr("Waiting others..."));
                ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/working.png"),ui->tableUpgraded->item(i,0)->text()));

                bool totalCompleted=true;
                for(int k=0;totalCompleted && k<rows;++k){
                    if(!completed[k]) totalCompleted=false;
                }

                if(totalCompleted){
                    for(int k=0;k<rows;++k){
                        QLabel *label = new QLabel();
                        label->setMovie(loadingMovie);
                        ui->tableUpgraded->setCellWidget(k,0,label);
                        ((QProgressBar*)ui->tableUpgraded->cellWidget(k,5))->setFormat(tr("Installing..."));
                    }

                    break;
                }
            }
        }
        break;
     case 2:
        rows=ui->tableUpgraded->rowCount();
        for(int i=0;i<rows;++i){
            if(ui->tableUpgraded->item(i,0)->text()!=QString("Upgrade") || completed[i+baseIndex]) continue;

            Package pkg;
            pkg.setName(ui->tableUpgraded->item(i,1)->text().split('/').at(1).toAscii().data());

            QString versions = ui->tableUpgraded->item(i,2)->text();
            QString remoteversion = versions.right(versions.size()-versions.indexOf('(')-1);
            remoteversion = remoteversion.left(remoteversion.size()-1);
            pkg.setRemoteVersion(remoteversion.toAscii().data());

            float reached = as->getProgressSize(&pkg);
            float total = ui->tableUpgraded->item(i,4)->text().toFloat()*1024;

            float speed=0;
            if(reached){
                if(baseSizes[i+baseIndex]!=-1){
                    speed=(reached-baseSizes[i+baseIndex])/((float)0.25);
                    ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setFormat(QString("%p% (")+QString::number((int)speed)+" KB/s)");
                }
                baseSizes[i+baseIndex]=reached;

            }

            float perc = total?(reached/(float)total)*100:100;
            ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setValue((int)perc);

            if(perc>0) ui->tableUpgraded->scrollToItem(ui->tableUpgraded->item(i,0));

            if(perc>=99){
                completed[i+baseIndex]=true;
                ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setValue(100);
                ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setFormat(tr("Waiting others..."));
                ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/working.png"),"Upgrade"));


                bool totalCompleted=true;
                for(int k=0;totalCompleted && k<rows;++k){
                    if(!completed[k+baseIndex]) totalCompleted=false;
                }

                if(totalCompleted){
                    for(int k=0;k<rows;++k){
                        QLabel *label = new QLabel();
                        label->setMovie(loadingMovie);
                        ui->tableUpgraded->setCellWidget(k,0,label);
                        ((QProgressBar*)ui->tableUpgraded->cellWidget(k,5))->setFormat(tr("Installing..."));
                    }

                    break;
                }
            }
        }
        break;
    }
}

bool MainWindow::outcomeEvaluator(){
    int count=ui->tableWidget->rowCount();
    bool outcome=false;
    for(int i=0;i<count && !outcome && (iOutcome.size() || rOutcome.size() || uOutcome.size());++i){
        QString actual=ui->tableWidget->item(i,2)->toolTip().trimmed();
        if(iOutcome.contains(actual)){
            iOutcome.removeOne(actual);
            if(ui->tableWidget->item(i,0)->text()=="Remote")outcome=true;
        }else if(uOutcome.contains(actual)){
            uOutcome.removeOne(actual);
            if(ui->tableWidget->item(i,0)->text()=="Upgradable")outcome=true;
        }else if(rOutcome.contains(actual)){
            rOutcome.removeOne(actual);
            if(!(ui->tableWidget->item(i,0)->text()=="Remote"))outcome=true;
        }
    }

    iOutcome.clear();
    uOutcome.clear();
    rOutcome.clear();

    return outcome;
}

void MainWindow::opFinished(){
    int op = asThread->getOp();
    int status = asThread->getStatus();
    std::list<Package*> *l = asThread->getList();
    int rows=ui->tableUpgraded->rowCount();

    switch(op){
    case 1:
        baseIndex = toI;
        toI=0;
        statusI=status;

        if(!statusI){
            for(int i=0;i<rows;++i){
                if(ui->tableUpgraded->item(i,0)->text()=="Upgrade" || ui->tableUpgraded->item(i,0)->text()==QString("Remove")) continue;
                ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/success.png"),"Install"));
            }
            delete l;
        }

        break;
    case 2:
        toU=0;
        statusU=status;

        if(!statusU){
            for(int i=0;i<rows;++i){
                if(ui->tableUpgraded->item(i,0)->text()=="Upgrade")
                    ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/success.png"),"Upgrade"));
            }
            delete l;
        }
        break;
    case 3:
        toR=0;
        statusR=status;

        if(!statusR){
            for(int i=0;i<rows;++i){
                if(ui->tableUpgraded->item(i,0)->text()==QString("Remove"))
                    ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/success.png"),"Remove"));
            }
            delete l;
        }
        break;
    }

    timerUpdate->stop();

    if(op>0&&op<4){
        if(!toI && !toU && !toR){
            as->removeListener(logger);
            delete logger;

            ui->stacked->setCurrentIndex(0);
            QCoreApplication::processEvents(QEventLoop::AllEvents,500);
            ui->mainToolBar->show();

            ui->editCancel->setEnabled(true);
            ui->editConfirm->setEnabled(true);

            ui->searchBar->clear();

            modified=0;
            applyEnabler();
            ui->showAll->setChecked(true);

            merging = false;

            ui->choicesGroup->setVisible(true);
            ui->backGroup->setHidden(true);

            addRows();

            QMessageBox done;
            bool status = outcomeEvaluator();//statusI+statusR+statusU;

            done.setText(status?tr("Errors during operations!"):tr("Success!"));
            done.setInformativeText(status?tr("Do you want to see operations logs?"):tr("All operations completed successfully!"));
            done.setIcon(status?QMessageBox::Critical:QMessageBox::Information);
            done.setStandardButtons(status?QMessageBox::Yes|QMessageBox::No:QMessageBox::Ok);
            if(status){
                int showLogs = done.exec();

                if(showLogs==QMessageBox::Yes){
                    QDialog logDialog;
                    QTextBrowser logText(&logDialog);
                    QPushButton ok(style()->standardIcon(QStyle::SP_DialogOkButton),tr("Continue"), &logDialog);
                    logDialog.setLayout(new QVBoxLayout());
                    std::ifstream logFile;
                    std::string buffer;

#ifdef unix
                    logFile.open("/var/log/appset.log");
#endif

                    while(std::getline(logFile,buffer)){
                        logText.append(QString("\n")+buffer.c_str());
                    }

                    logDialog.layout()->addWidget(&logText);
                    logDialog.layout()->addWidget(&ok);
                    connect(&ok,SIGNAL(clicked()),&logDialog,SLOT(close()));

                    logDialog.setFixedSize(450,300);

                    logDialog.exec();

                    disconnect(&logDialog);
                }
            }else ui->statusBar->showMessage(tr("All operations completed successfully!"),5000);//done.show();
        }else editConfirm();
    }
}

void MainWindow::editConfirm(){
    timerConfirm->stop();
    ui->editConfirm->setText(tr("Confirm"));

    ui->editCancel->setDisabled(true);
    ui->editConfirm->setDisabled(true);
    Package *p;

    int rows=ui->tableUpgraded->rowCount();

    if(toR){
        std::list<Package*> *prem=new std::list<Package*>();
        for(int i=0;i<rows;++i){
            if(ui->tableUpgraded->item(i,0)->text()==QString("Remove")){
                p=new Package();
                p->setName(ui->tableUpgraded->item(i,1)->text().trimmed().toAscii().data());
                prem->insert(prem->end(), p);
                ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/waiting.png"),"Remove"));

                rOutcome << ui->tableUpgraded->item(i,1)->toolTip().trimmed();
            }
        }

        asThread->setList(prem);
        asThread->setOp(3);
        pkgs=prem;
        asThread->start();
    }else if(toI){
        std::list<Package*> *pinst=new std::list<Package*>();
        for(int i=0;i<rows;++i){
            if(ui->tableUpgraded->item(i,0)->text()==QString("Remove") || ui->tableUpgraded->item(i,0)->text()=="Upgrade") continue;
            p=new Package();
            if(!local)p->setName(ui->tableUpgraded->item(i,1)->toolTip().trimmed().toAscii().data());
            else p->setName(ui->tableUpgraded->item(i,1)->text().trimmed().toAscii().data());
            pinst->insert(pinst->end(), p);
            ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/waiting.png"),ui->tableUpgraded->item(i,0)->text()));

            iOutcome << ui->tableUpgraded->item(i,1)->toolTip().trimmed();
        }

        asThread->setList(pinst);
        asThread->setOp(1);
        asThread->local=local;
        pkgs=pinst;
        asThread->start();
    }else if(toU){
        std::list<Package*> *pupgr=new std::list<Package*>();
        int rows = ui->tableUpgraded->rowCount();
        /*for(int i=0;i<rows;++i){
            if(ui->tableWidget->item(i,0)->text()=="Upgradable"){
                p=new Package();
                p->setName(ui->tableWidget->item(i,2)->text().trimmed().toAscii().data());
                pupgr->insert(pupgr->end(),p);
            }
        }
        rows=ui->tableUpgraded->rowCount();*/
        for(int i=0;i<rows;++i){
            if(ui->tableUpgraded->item(i,0)->text()=="Upgrade"){
                ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/waiting.png"),"Upgrade"));
                p=new Package();
                p->setName(ui->tableUpgraded->item(i,1)->text().trimmed().toAscii().data());
                pupgr->insert(pupgr->end(),p);

                uOutcome << ui->tableUpgraded->item(i,1)->toolTip().trimmed();
            }
        }

        asThread->setList(pupgr);
        asThread->setOp(2);
        pkgs=pupgr;
        asThread->start();
    }

    if(toU || toR || toI){
        timerUpdate->start(250);
        ui->choicesGroup->setHidden(true);
        if(this->showBackOut) ui->backGroup->setVisible(true);
    }
}

void MainWindow::timeFilter(){
    asyncFilter(ui->searchBar->text());
    ui->searchBar->setFocus();
}

void MainWindow::timerFired(QString s){    
    timer2->stop();
    timer2->setSingleShot(true);
    timer2->start(this->sbdelay);s="";
}


void MainWindow::editCancel(){
    timerConfirm->stop();

    ui->stacked->setCurrentIndex(0);
    ui->mainToolBar->show();

    as->removeListener(logger);

    delete logger;

    if(local)addRows();
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
    else{ filterPressed(); return;}

    filter=filter.trimmed();
    filter.replace(' ','|');

    int index = ui->comboBox->currentIndex();

    loadingDialog->show();

    ui->centralWidget->setEnabled(false);

    if(index==2){
        for(int i=0;i<rows;++i){
            if(!ui->tableWidget->item(i,2) || !ui->tableWidget->item(i,5) || (!isExpert && ui->tableWidget->item(i,2)->text().contains(expert)) ||
               (ui->tableWidget->item(i,2)->text().contains(category_exclude) || ui->tableWidget->item(i,5)->text().contains(category_exclude))
                || (!ui->tableWidget->item(i,2)->text().contains(category) && !ui->tableWidget->item(i,5)->text().contains(category))
                || (!ui->tableWidget->item(i,2)->text().contains(QRegExp(filter,Qt::CaseInsensitive)) && !ui->tableWidget->item(i,5)->text().contains(QRegExp(filter,Qt::CaseInsensitive)))){
                ui->tableWidget->hideRow(i);

                if(!(i%333)){
                    loadingBar->setValue(i*100/(float)rows);
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 33);
                }
            }
        }
    }else{
        index=index?5:2;

        for(int i=0;i<rows;++i){
            if(!ui->tableWidget->item(i,2) || !ui->tableWidget->item(i,5) || (!isExpert && ui->tableWidget->item(i,2)->text().contains(expert)) ||
               (ui->tableWidget->item(i,2)->text().contains(category_exclude) || ui->tableWidget->item(i,5)->text().contains(category_exclude)) ||
                (!ui->tableWidget->item(i,2)->text().contains(category) && !ui->tableWidget->item(i,5)->text().contains(category))
                || !ui->tableWidget->item(i,index)->text().contains(QRegExp(filter,Qt::CaseInsensitive))){
                ui->tableWidget->hideRow(i);

                if(!(i%333)){
                    loadingBar->setValue(i*100/(float)rows);
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 33);
                }
            }
        }
    }

    loadingDialog->hide();

    ui->centralWidget->setEnabled(true);

    /*if(!isExpert){
        for(int i=0;i<rows;++i){
            if(ui->tableWidget->item(i,1)->text().contains(expert))
                ui->tableWidget->hideRow(i);
        }
    }*/

    ui->tableWidget->sortByColumn(2,Qt::AscendingOrder);

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

    loadingBar->setValue(0);
}

void MainWindow::filterPressed(){
    if(ui->showAll->isChecked()) showAll(true);
    else if(ui->showInstalled->isChecked()) showInstalled(true);
    else if(ui->showNotInstalled->isChecked()) showNotInstalled(true);
    else if(ui->showUpgradable->isChecked()) showUpgradable(true);
}

void MainWindow::applyEnabler(){
    applyAction->setEnabled(modified>0);
    openLocalAction->setEnabled(modified==0);
}

void MainWindow::install(){
    ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(QIcon(":pkgstatus/install.png"),"Install"));
    modified++;

    Package p;
    std::list<AS::Package*> *pkgs;
    p.setName(ui->tableWidget->item(currentPacket,2)->toolTip().trimmed().toAscii().data());
    if((pkgs = as->checkDeps(&p,true))){
        for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
            //XXX
            QString name = QString((*it)->getRepository().c_str()).trimmed()+QString("/")+QString((*it)->getName().c_str()).trimmed();
            QString pname = QString(p.getName().c_str());
            if(name!=pname){
                for(int i=0;i<ui->tableWidget->rowCount();++i){
                    if(ui->tableWidget->item(i,2)->toolTip()==name){
                        currentPacket=i;
                        if(!instaDeps.contains(name,pname))instaDeps.insert(name, pname);
                        this->install();
                    }
                }
            }
            delete(*it);
        }
        delete pkgs;
    }

    applyEnabler();
    ui->statusBar->showMessage(tr("Pending changes:")+QString::number(modified),2000);
}

void MainWindow::remove(){
    Package p;
    QStringList req;
    p.setName(ui->tableWidget->item(currentPacket,2)->text().trimmed().toAscii().data());
    QString pname(p.getName().c_str());
    if((pkgs = as->checkDeps(&p,false))){
        for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
            QString name = QString((*it)->getName().c_str()).trimmed();            
            if(name!=pname)
                req << name;
            delete(*it);
        }
        delete pkgs;
    }
    int res = QMessageBox::Yes;
    if(req.size()){
        QMessageBox reqMes;
        reqMes.setText(tr("Some installed packages require") + QString(" ") + pname + QString(":"));
        reqMes.setInformativeText(req.join("\n")+QString("\n\n")+tr("Do you want to proceed anyway (removing them too)?"));
        reqMes.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        reqMes.setIcon(QMessageBox::Warning);
        res = reqMes.exec();
    }

    if(res == QMessageBox::Yes){
        ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(QIcon(":pkgstatus/remove.png"),"Remove"));
        modified++;

        //Select deps too
        for(int j=0;j<req.size();++j){
            QString cur=req.at(j);
            for(int i=0;i<ui->tableWidget->rowCount();++i){
                if(ui->tableWidget->item(i,2)->text()==cur){
                    currentPacket=i;
                    if(!remDeps.contains(cur,pname)) remDeps.insert(cur, pname);
                    this->remove();
                }
            }
        }

        applyEnabler();
    }

    ui->statusBar->showMessage(tr("Pending changes:")+QString::number(modified),2000);
}

void MainWindow::upgrade(){
    ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(style()->standardIcon(QStyle::SP_ArrowUp),"Upgrade"));
    modified++;

    Package p;
    std::list<AS::Package*> *pkgs;
    p.setName(ui->tableWidget->item(currentPacket,2)->toolTip().trimmed().toAscii().data());
    if((pkgs = as->checkDeps(&p,true))){
        for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
            QString name = QString((*it)->getRepository().c_str()).trimmed()+QString("/")+QString((*it)->getName().c_str()).trimmed();
            QString pname = QString(p.getName().c_str());
            if(name!=pname){
                for(int i=0;i<ui->tableWidget->rowCount();++i){
                    if(ui->tableWidget->item(i,2)->toolTip()==name){
                        currentPacket=i;
                        this->install();
                    }
                }
            }
            delete(*it);
        }
        delete pkgs;
    }


    applyEnabler();

    ui->statusBar->showMessage(tr("Pending changes:")+QString::number(modified),2000);
}

void MainWindow::notInstall(){
    QString dname =  ui->tableWidget->item(currentPacket,2)->toolTip();
    QList<QString> requirers = instaDeps.values(dname);

    int res = QMessageBox::Yes;

    if(requirers.size()){
        //Ask for confirm
        QMessageBox reqMes;
        reqMes.setText(tr("These selected for install packages require") + QString(" ") + dname + QString(":"));
        reqMes.setInformativeText(QStringList(requirers).join("\n")+QString("\n\n")+tr("Do you want to proceed anyway (clearing them too)?"));
        reqMes.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        reqMes.setIcon(QMessageBox::Warning);
        res = reqMes.exec();
    }

    if(res == QMessageBox::Yes){
        ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(QIcon(":pkgstatus/unchecked.png"),"Remote"));
        modified--;
        for(int i=0;i<requirers.size();++i){
            instaDeps.remove(dname,requirers.at(i));
            for(int j=0;j<ui->tableWidget->rowCount();++j){
                if(ui->tableWidget->item(j,2)->toolTip() == requirers.at(i)){
                    currentPacket = j;
                    break;
                }
            }
            this->notInstall();
        }
    }

    Package p;
    p.setName(dname.trimmed().toAscii().data());
    std::list<AS::Package*> *pkgs = as->checkDeps(&p,true);
    QMessageBox reqMes;
    QStringList plist;
    if(pkgs && pkgs->size()){
        for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
            QString name = QString((*it)->getRepository().c_str()).trimmed()+QString("/")+QString((*it)->getName().c_str()).trimmed();
            if(name!=dname){
                for(int i=0;i<ui->tableWidget->rowCount();++i){
                    if( ui->tableWidget->item(i,2)->toolTip()==name &&
                          ui->tableWidget->item(i,0)->text()=="Install"){
                        plist << name;
                    }
                }
            }
        }

        if(plist.size()){
            reqMes.setText(tr("These packages were selected as dependencies of")+QString(" ")+dname+QString(":"));
            reqMes.setInformativeText(plist.join("\n")+QString("\n\n")+tr("Do you want to clear them too?"));
            reqMes.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            reqMes.setIcon(QMessageBox::Question);            

            int resp=0;
            if(isExpert)
                resp=reqMes.exec();
            for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                QString name = QString((*it)->getRepository().c_str()).trimmed()+QString("/")+QString((*it)->getName().c_str()).trimmed();
                if(name!=dname){
                    for(int i=0;i<ui->tableWidget->rowCount();++i){
                        if(ui->tableWidget->item(i,2)->toolTip()==name){
                            instaDeps.remove(name,dname);
                            if(!isExpert || resp == QMessageBox::Yes){
                                currentPacket = i;
                                this->notInstall();
                            }
                        }
                    }
                }
                delete(*it);
            }
            delete pkgs;
        }
    }

    applyEnabler();

    ui->statusBar->showMessage(tr("Pending changes:")+QString::number(modified),2000);
}

void MainWindow::notRemove(){
    QString dname =  ui->tableWidget->item(currentPacket,2)->text();
    QList<QString> requirers = remDeps.values(dname);

    int res = QMessageBox::Yes;

    if(requirers.size()){
        //Ask for confirm
        QMessageBox reqMes;
        reqMes.setText(tr("These selected for removal packages are required by") + QString(" ") + dname + QString(":"));
        reqMes.setInformativeText(QStringList(requirers).join("\n")+QString("\n\n")+tr("Do you want to proceed anyway (canceling their removal too)?"));
        reqMes.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        reqMes.setIcon(QMessageBox::Warning);
        res = reqMes.exec();
    }

    if(res == QMessageBox::Yes){
        ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(QIcon(":pkgstatus/checked.png"),"Installed"));
        modified--;
        for(int i=0;i<requirers.size();++i){
            remDeps.remove(dname,requirers.at(i));
            for(int j=0;j<ui->tableWidget->rowCount();++j){
                if(ui->tableWidget->item(j,2)->text() == requirers.at(i)){
                    currentPacket = j;
                    break;
                }
            }
            this->notRemove();
        }
    }

    Package p;
    p.setName(dname.trimmed().toAscii().data());
    std::list<AS::Package*> *pkgs = as->checkDeps(&p,false);
    QMessageBox reqMes;
    QStringList plist;
    if(pkgs && pkgs->size()){
        for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
            QString name = QString((*it)->getName().c_str()).trimmed();
            if(name!=dname){
                for(int i=0;i<ui->tableWidget->rowCount();++i){
                    if( ui->tableWidget->item(i,2)->text()==name &&
                          ui->tableWidget->item(i,0)->text()=="Remove"){
                        plist << name;
                    }
                }
            }
        }

        if(plist.size()){
            reqMes.setText(tr("These packages were selected for removal because they need")+QString(" ")+dname+QString(":"));
            reqMes.setInformativeText(plist.join("\n")+QString("\n\n")+tr("Do you want to clear their removal too?"));
            reqMes.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            reqMes.setIcon(QMessageBox::Question);

            int resp=0;
            if(isExpert)
                resp=reqMes.exec();
            for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                QString name = QString((*it)->getName().c_str()).trimmed();
                if(name!=dname){
                    for(int i=0;i<ui->tableWidget->rowCount();++i){
                        if(ui->tableWidget->item(i,2)->text()==name){
                            remDeps.remove(name,dname);
                            if(!isExpert || resp == QMessageBox::Yes){
                                currentPacket = i;
                                this->notRemove();
                            }
                        }
                    }
                }
                delete(*it);
            }
            delete pkgs;
        }
    }

    applyEnabler();
    ui->statusBar->showMessage(tr("Pending changes:")+QString::number(modified),2000);
}

void MainWindow::notUpgrade(){
    ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(QIcon(":pkgstatus/upgrade.png"),"Upgradable"));
    modified--;
    applyEnabler();
    ui->statusBar->showMessage(tr("Pending changes:")+QString::number(modified),2000);
}

void MainWindow::confirm(){
    logger=new ASLogger();
    as->addListener(logger);

    ui->stacked->setCurrentIndex(1);
    ui->mainToolBar->hide();

    ui->tableUpgraded->clearContents();
    ui->tableUpgraded->setRowCount(0);

    int in=0,r=0,u=0;
    int rows=ui->tableWidget->rowCount();
    for(int i=0;i<rows;++i){

        if(!isExpert && ui->tableWidget->item(i,0)->text()=="Upgradable" && ui->tableWidget->item(i,2)->text().contains(expert)){
            currentPacket=i;
            this->upgrade();
            modified++;
        }

        if(ui->tableWidget->item(i,0)->text()==QString("Remove")){
                    ui->tableUpgraded->insertRow(r);
                    ui->tableUpgraded->setItem(r,0,new QTableWidgetItem(*ui->tableWidget->item(i,0)));
                    ui->tableUpgraded->item(r,0)->setText("Remove");
                    ui->tableUpgraded->setItem(r,1,new QTableWidgetItem(*ui->tableWidget->item(i,2)));

                    Package p;
                    p.setName(ui->tableUpgraded->item(r,1)->text().trimmed().toAscii().data());
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

                    QProgressBar *prog=new QProgressBar();
                    prog->setValue(0);
                    prog->hide();
                    ui->tableUpgraded->setCellWidget(r,5,prog);

                    ui->tableUpgraded->setItem(r,3,new QTableWidgetItem(deps.join(" ")));
                    ui->tableUpgraded->setItem(r,4,new QTableWidgetItem(QString::number(ui->tableWidget->item(i,7)->text().toFloat()/1024)));

                    ui->tableUpgraded->setItem(r++,2,new QTableWidgetItem(*ui->tableWidget->item(i,3)));
        }else if(ui->tableWidget->item(i,0)->text()=="Install" && instaDeps.value(ui->tableWidget->item(i,2)->toolTip())==""){
            ui->tableUpgraded->insertRow(in);
            ui->tableUpgraded->setItem(in,0,new QTableWidgetItem(*ui->tableWidget->item(i,0)));
            ui->tableUpgraded->item(in,0)->setText(ui->tableWidget->item(i,7)->text());
            ui->tableUpgraded->setItem(in,1,new QTableWidgetItem(*ui->tableWidget->item(i,2)));
            ui->tableUpgraded->setItem(in,2,new QTableWidgetItem(*ui->tableWidget->item(i,4)));

            ui->tableUpgraded->item(in,1)->setText(ui->tableUpgraded->item(in,1)->toolTip());
            Package p;
            int dsize = ui->tableWidget->item(i,7)->text().toFloat();
            if(!local)p.setName(ui->tableUpgraded->item(in,1)->toolTip().trimmed().toAscii().data());
            else p.setName(ui->tableWidget->item(i,4)->text().toAscii().data());
            QStringList deps;

            if((pkgs = as->checkDeps(&p,true,false,local))){
                for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                    QString name = QString((*it)->getRepository().c_str()).trimmed()+QString("/")+QString((*it)->getName().c_str()).trimmed();

                    if(name!=ui->tableWidget->item(i,2)->toolTip().trimmed()){
                        deps << name;
                        dsize+=QString((*it)->getRemoteVersion().c_str()).toFloat()/1024;
                    }
                    delete(*it);
                }
                delete pkgs;
            }


            ui->tableUpgraded->setItem(in,3,new QTableWidgetItem(deps.join(" ")));


            QProgressBar *prog=new QProgressBar();
            prog->setValue(0);
            //prog->setMaximumWidth(200);
            ui->tableUpgraded->setCellWidget(in,5,prog);

            //ui->tableUpgraded->setItem(in++,4,new QTableWidgetItem(*ui->tableWidget->item(i,4)));
            ui->tableUpgraded->setItem(in++,4,new QTableWidgetItem(QString::number(dsize/(float)1024)));
        }else if(ui->tableWidget->item(i,0)->text()=="Upgrade"){
            ui->tableUpgraded->insertRow(u);            
            ui->tableUpgraded->setItem(u,0,new QTableWidgetItem(*ui->tableWidget->item(i,0)));
            ui->tableUpgraded->item(u,0)->setText("Upgrade");
            ui->tableUpgraded->setItem(u,1,new QTableWidgetItem(*ui->tableWidget->item(i,2)));
            ui->tableUpgraded->setItem(u,2,new QTableWidgetItem(ui->tableWidget->item(i,3)->text()+QString(" (")+ui->tableWidget->item(i,4)->text()+QString(")")));

            ui->tableUpgraded->item(u,1)->setText(ui->tableUpgraded->item(u,1)->toolTip());
            //ui->tableUpgraded->setItem(u,3,new QTableWidgetItem(*ui->tableWidget->item(i,3)));

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
            ui->tableUpgraded->setItem(u++,4,new QTableWidgetItem(QString::number(ui->tableWidget->item(i,7)->text().toFloat()/1024)));

//            ui->tableUpgraded->showRow(u++);
        }
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

    confirmRemaining=15;

    if(confirmCountdown){
        ui->editConfirm->setText(tr("Confirm")+QString(" (15)"));
        timerConfirm->start(1000);
    }else{
        ui->editConfirm->setText(tr("Confirm"));
    }
}

void MainWindow::changeStatus(int row, int col){    
    if(ui->tableWidget->item(row,6) && ui->tableWidget->item(row,2)){
        if(ui->webView && ui->webView->isEnabled()){
            ui->webView->stop();
            ui->webView->setUrl(QUrl::fromUserInput(ui->tableWidget->item(row,6)->text().trimmed()));
        }
        Package p;
        if(ui->tableWidget->item(row,0)->text()=="Installed" || ui->tableWidget->item(row,0)->text()=="Remove")
            p.setName(ui->tableWidget->item(row,2)->text().trimmed().toAscii().data());
        else
            p.setName(ui->tableWidget->item(row,2)->toolTip().trimmed().toAscii().data());
        ui->infoText->clear();

        if(ui->tableWidget->item(row,0)->text()!="Installed" && ui->tableWidget->item(row,0)->text()!="Remove" )
            ui->infoText->append(QString("<b>")+tr("Repository")+tr(": </b>")+ui->tableWidget->item(row,1)->text());

        if(!(ui->tableWidget->item(row,0)->text()=="Installed" || ui->tableWidget->item(row,0)->text()=="Remove"))
            ui->infoText->append(QString(tr("<b>Size: </b>"))+ui->tableWidget->item(row,7)->text()+" KB");

        ui->infoText->append(QString(tr("<b>URL: </b>"))+ui->tableWidget->item(row,6)->text());
        ui->infoText->append(QString("<a href=\"")+ui->tableWidget->item(row,6)->text()+QString("\" >")+QString(tr("(Watch the full site)"))+QString("</a>"));

        if(!(ui->tableWidget->item(row,0)->text()=="Installed" || ui->tableWidget->item(row,0)->text()=="Remove")){
            ui->infoText->append(tr("<b>Requires:</b>"));
            if((pkgs = as->checkDeps(&p,true))){
                for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                    QString name;
                    name = QString((*it)->getRepository().c_str()).trimmed()+QString("/")+QString((*it)->getName().c_str()).trimmed();
                    if(name!=QString(p.getName().c_str())){
                        ui->infoText->append(name+QString(" (")+QString::number(QString((*it)->getRemoteVersion().c_str()).toInt()/1024/1024)+QString(" MB)"));
                    }
                    delete(*it);
                }
                delete pkgs;
            }
        }else{
            ui->infoText->append(tr("<b>Required By:</b>"));
            if((pkgs = as->checkDeps(&p,false))){
                for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                    QString name;
                    if(ui->tableWidget->item(row,0)->text()=="Installed" || ui->tableWidget->item(row,0)->text()=="Remove")
                        name = QString((*it)->getName().c_str()).trimmed();
                    else
                        name = QString((*it)->getRepository().c_str()).trimmed()+QString("/")+QString((*it)->getName().c_str()).trimmed();
                    if(name!=QString(p.getName().c_str())){
                        ui->infoText->append(name);
                    }
                    delete(*it);
                }
                delete pkgs;
            }
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

    merging = false;

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

    QString param("");
    if(!argsParsed && qApp->argc()>1){
        param=QString((qApp->argv())[1]);
        argsParsed=true;
        openLocal(param);
        return;
    }

    tpack=ipack=upack=epack=0;
    local=false;

    int rows=0,upgradables=0;

    ui->centralWidget->setDisabled(true);
    ui->mainToolBar->setDisabled(true);

    //ui->stacked->setCurrentIndex(2);
    loadingDialog->show();
    loadingBar->setValue(0);

    ui->tableWidget->setSortingEnabled(false);

    timer2->stop();

    QCoreApplication::processEvents(QEventLoop::AllEvents, 500);

    QTableWidgetItem *newItem;


    //ui->tableWidget->clearContents();
    int count = ui->tableWidget->rowCount();
    for(int i=0;i<count;++i)ui->tableWidget->removeRow(0);
    //ui->tableWidget->setRowCount(0);

    if(!merging || (pkgs=as->queryRemote(as_MERGE_QUERIES))==0){
        StatusBarUpdater sbu(ui->statusBar);
        sbu.setPB(loadingBar);

        as->addListener(&sbu);
        pkgs = as->queryRemote(as_QUERY_ALL_INFO | as_EXPERT_QUERY);
        ui->tableWidget->setRowCount(pkgs->size());
        tpack=pkgs->size();
        as->removeListener(&sbu);

        AsThread t2(as);
        t2.setOp(4);
        t2.start();

        //loadingBar->setValue(30);

        int i=0;        

        for(std::list<Package*>::iterator it=pkgs->begin(); it!=pkgs->end(); it++){
            Package *pkg = *it;

            //ui->tableWidget->insertRow(i);
            newItem = new QTableWidgetItem(QIcon(":pkgstatus/unchecked.png"),"Remote");
            newItem->setToolTip(tr("Not Installed"));
            ui->tableWidget->setItem(i,0, newItem);            
            newItem = new QTableWidgetItem(pkg->getRepository().c_str());
            ui->tableWidget->setItem(i,1, newItem);
            //XXX change for other distributions
            newItem = new QTableWidgetItem(pkg->getName().c_str());
            newItem->setToolTip((QString(pkg->getRepository().c_str())+QString("/")+QString(pkg->getName().c_str())));
            ui->tableWidget->setItem(i,2, newItem);
            newItem = new QTableWidgetItem(pkg->getRemoteVersion().c_str());
            ui->tableWidget->setItem(i,4, newItem);
            newItem = new QTableWidgetItem(pkg->getDescription().c_str());
            ui->tableWidget->setItem(i,5, newItem);
            newItem = new QTableWidgetItem(pkg->getURL().c_str());
            ui->tableWidget->setItem(i,6, newItem);
            newItem = new QTableWidgetItem(QString::number(pkg->getSize()));
            ui->tableWidget->setItem(i,7, newItem);

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
        ipack=pkgs->size();

        //loadingBar->setValue(60);

        ui->statusBar->showMessage(tr("SEARCHING CORRESPONDENCES..."));

        int k=0;
        rows = ui->tableWidget->rowCount();
        QList<int> toRemove;
        for(std::list<Package*>::iterator it=pkgs->begin(); it!=pkgs->end(); it++){
            Package *pkg = *it;

            int found=-1;
            QTableWidgetItem* versionMatch;
            for(int index=0;index<rows;++index){
                //XXX Installed local?
                if(ui->tableWidget->item(index,2)->text()==QString(pkg->getName().c_str())){
                    /*if(found!=-1){
                        if((((QTNIXEngine*)as)->compareVersions(ui->tableWidget->item(index,4)->text(),versionMatch->text()))<0){
                            toRemove.insert(toRemove.end(),index);
                        }else{
                            toRemove.insert(toRemove.end(),found);
                            found=index;
                            versionMatch=ui->tableWidget->item(index,4);
                        }
                    }else{
                        versionMatch=ui->tableWidget->item(index,4);
                        found=index;
                    }*/
                    found=index;
                    newItem = new QTableWidgetItem(QIcon(":pkgstatus/checked.png"),"Installed");
                    newItem->setToolTip(tr("Installed"));
                    ui->tableWidget->setItem(found,0, newItem);
                    newItem = new QTableWidgetItem(pkg->getLocalVersion().c_str());
                    ui->tableWidget->setItem(found,3, newItem);

                    //ui->tableWidget->item(found,1)->setText("");

                    //ipack++;
                }
            }

            if(found!=-1){
                /*
                */;
            }else{
                ui->tableWidget->insertRow(i);
                newItem = new QTableWidgetItem(QIcon(":pkgstatus/checked.png"),"Installed");
                newItem->setToolTip(tr("Installed (external)"));
                ui->tableWidget->setItem(i,0, newItem);
                //newItem = new QTableWidgetItem(pkg->getRepository().c_str());
                //ui->tableWidget->setItem(i,1, newItem);
                newItem = new QTableWidgetItem(pkg->getName().c_str());
                newItem->setToolTip((QString(pkg->getRepository().c_str())+QString("/")+QString(pkg->getName().c_str())));
                ui->tableWidget->setItem(i,2, newItem);
                newItem = new QTableWidgetItem(pkg->getLocalVersion().c_str());
                ui->tableWidget->setItem(i,3, newItem);
                newItem = new QTableWidgetItem((QString(" (")+tr("External")+QString(")")));
                ui->tableWidget->setItem(i,4, newItem);
                newItem = new QTableWidgetItem(pkg->getDescription().c_str());
                ui->tableWidget->setItem(i,5, newItem);
                newItem = new QTableWidgetItem(pkg->getURL().c_str());
                ui->tableWidget->setItem(i,6, newItem);
                newItem = new QTableWidgetItem(QString::number(pkg->getSize()));
                ui->tableWidget->setItem(i,7, newItem);
                i++;
                rows++;
                //ipack++;
                epack++;
            }

            k++;

            if(!(k%11)){
                loadingBar->setValue(50+k*45/pkgs->size());
                QCoreApplication::processEvents(QEventLoop::AllEvents, 33);
            }

            delete pkg;
        }

        QCoreApplication::processEvents(QEventLoop::AllEvents, 33);

        ui->statusBar->showMessage("CHECKING UPGRADABLES", 3000);

        rows = ui->tableWidget->rowCount();

        Package *pp=new Package(true);
        pp->setName("");
        std::list<Package*> *ups = as->checkDeps(pp,true,true);//as->queryLocal(as_QUERY_UPGRADABLE|as_EXPERT_QUERY);
        delete pp;
        if(ups){
            std::list<Package*>::iterator upsit = ups->begin();
            //XXX cycle on ups instead of rows
            for(int i=0;i<rows;++i){
                if(ui->tableWidget->item(i,0) && (ui->tableWidget->item(i,0)->text()=="Installed" || ui->tableWidget->item(i,0)->text()=="Remove")
                     && ui->tableWidget->item(i,3) && ui->tableWidget->item(i,4) && ui->tableWidget->item(i,4)->text()!=(QString(" (")+tr("External")+QString(")"))){

                    bool found=false;
                    while(!found && upsit!=ups->end()){
                        Package *pkgup=*upsit;
                        if(ui->tableWidget->item(i,2)->text()==QString(pkgup->getName().c_str()) &&
                                ui->tableWidget->item(i,1)->text()==QString(pkgup->getRepository().c_str())) found=true;
                        else upsit++;
                    }
                    if(found){
                        newItem = new QTableWidgetItem(QIcon(":pkgstatus/upgrade.png"),"Upgradable");
                        newItem->setToolTip(tr("Upgradable"));
                        upgradables++;
                        ui->tableWidget->setItem(i,0,newItem);
                        QCoreApplication::processEvents(QEventLoop::AllEvents, 33);

                        delete *upsit;
                        ups->remove(*upsit);

                        upack++;
                    }

                    if(ups)upsit=ups->begin();
                }
                loadingBar->setValue(95+i*5/rows);
            }


            delete ups;
        }
        delete pkgs;
    }else{
        std::list<AS::Package*>::iterator it=pkgs->begin();

        tpack=rows = pkgs->size();
        ui->tableWidget->setRowCount(rows);
        int i=0;

        Package *pp=new Package(true);
        pp->setName("");
        std::list<Package*> *ups = as->checkDeps(pp,true,true);//as->queryLocal(as_QUERY_UPGRADABLE|as_EXPERT_QUERY);
        delete pp;
        std::list<Package*>::iterator upsit;
        if(ups) upsit = ups->begin();

        while(it!=pkgs->end()){
            bool installed=false;
            Package *pkg = *it;

            if(pkg->isInstalled()){
                ipack++;
                bool found=false;
                while(ups && !found && upsit!=ups->end()){
                    Package *pkgup=*upsit;
                    if(pkgup->getName().compare(pkg->getName()) == 0 &&
                            QString(pkg->getRepository().c_str())==QString(pkgup->getRepository().c_str())) found=true;
                    else upsit++;
                }
                if(found){
                    newItem = new QTableWidgetItem(QIcon(":pkgstatus/upgrade.png"),"Upgradable");
                    newItem->setToolTip(tr("Upgradable"));
                    upgradables++;

                    delete *upsit;
                    ups->remove(*upsit);
                    upack++;
                }else{
                    newItem = new QTableWidgetItem(QIcon(":pkgstatus/checked.png"),"Installed");
                    newItem->setToolTip(tr("Installed"));
                    installed=true;
                }
                if(ups)upsit=ups->begin();
            }else{
                newItem = new QTableWidgetItem(QIcon(":pkgstatus/unchecked.png"),"Remote");
                newItem->setToolTip(tr("Not Installed"));
            }
            ui->tableWidget->setItem(i,0, newItem);

            if(!installed){
                newItem = new QTableWidgetItem(pkg->getRepository().c_str());
                ui->tableWidget->setItem(i,1, newItem);
            }

            newItem = new QTableWidgetItem(pkg->getName().c_str());
            newItem->setToolTip((QString(pkg->getRepository().c_str())+QString("/")+QString(pkg->getName().c_str())));
            ui->tableWidget->setItem(i,2, newItem);

            if(pkg->isInstalled()){
                newItem = new QTableWidgetItem(pkg->getLocalVersion().c_str());
                ui->tableWidget->setItem(i,3, newItem);
            }

            if(pkg->getRemoteVersion().compare("NO INFO")){
                newItem = new QTableWidgetItem(pkg->getRemoteVersion().c_str());
            }else{
                newItem = new QTableWidgetItem((QString(" (")+tr("External")+QString(")")));
                epack++;
            }
            ui->tableWidget->setItem(i,4, newItem);

            newItem = new QTableWidgetItem(pkg->getDescription().c_str());
            ui->tableWidget->setItem(i,5, newItem);

            newItem = new QTableWidgetItem(pkg->getURL().c_str());
            ui->tableWidget->setItem(i,6, newItem);

            newItem = new QTableWidgetItem(QString::number(pkg->getSize()));
            ui->tableWidget->setItem(i,7, newItem);

            i++;

            delete pkg;

            if(!(i%20)){
                loadingBar->setValue(i*100/pkgs->size());
                QCoreApplication::processEvents(QEventLoop::AllEvents, 33);
            }

            it++;
        }

        delete pkgs;
        if(ups)delete ups;
    }

    markAction->setEnabled(upgradables>0);

    markAction->setText(tr("Mark all upgrades")+QString(" (")+QString::number(upgradables)+QString(")"));

#ifdef unix
    int cacheSize = ((QTNIXEngine*)as)->cacheSize();
    cleanAction->setEnabled(cacheSize>0);
    if(cacheSize>0){
        cleanAction->setText(tr("Clean cache")+QString(" (")+QString::number(cacheSize)+QString(" MB)"));
    }
#endif

    //ui->statusBar->showMessage(QString("INFO UPDATED: ")+QString::number(rows)+QString(" PACKAGES SHOWN"), 10000);
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

    merging = true;

    //ui->tableWidget->sortByColumn(1,Qt::AscendingOrder);

    ui->tLabel->setText(QString::number(tpack));
    ui->uLabel->setText(QString::number(upack));
    ui->iLabel->setText(QString::number(ipack));
    ui->eLabel->setText(QString::number(epack));

    if(ui->tabWidget->tabText(1)==tr("All"))ui->tabWidget->setTabText(1, tr("All")+QString(" (")+QString::number(ui->tableWidget->rowCount())+QString(")"));
}

#include <sys/types.h>
#include <signal.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <errno.h>

MainWindow::~MainWindow(){
    delete ui;

#ifdef unix
    delete (AS::QTNIXEngine*)as;

    std::ifstream helper_pid;
    helper_pid.open("/var/run/ashelper.pid");
    int pid = 0;
    helper_pid >> pid;
    if(pid){
        kill(pid, SIGUSR1);
    }
    helper_pid.close();
#endif
}
