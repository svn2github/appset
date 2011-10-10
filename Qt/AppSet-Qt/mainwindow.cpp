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

#include <QSplitter>
#include <QWidgetList>
#include <QDir>
#include <QFile>
#include <QWebHistory>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), currentReply(0){

    ui->setupUi(this);

    view=0;
    pkgs=0;

    fileTreeModel = 0;

    merging = true;

    local=false;

    ui->webView->history()->setMaximumItemCount(0);
    ui->webView_2->history()->setMaximumItemCount(0);

    pcomp=0;

    ui->cancelOps->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    connect(ui->cancelOps,SIGNAL(clicked()),this,SLOT(cancelOps()));

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
    as = new AS::QTNIXEngine(new InputProvider());

    int errno=0;
    pp=privilegedExecuter(qApp->argc(),qApp->argv());
    bool privileged=pp>0 && pp!=9 && pp!=11;
    if(privileged){
        this->setWindowTitle("AppSet-Qt (SUPERUSER)");        
    }else{
        system("appsettray-qt &");

        ui->cancelOpsWidget->hide();
    }
    argsParsed=pp!=4 && pp!=0;
    if((errno=((AS::QTNIXEngine*)as)->configure("/etc/appset.conf",privileged?"/tmp/as.tmp":"/tmp/asuser.tmp",!privileged))){
        if(privileged){
            QMessageBox errMsg;
            errMsg.setText(tr("Errors while initializing the system!"));
            errMsg.setInformativeText(((AS::QTNIXEngine*)as)->getConfErrStr(errno));
            errMsg.setIcon(QMessageBox::Critical);
            errMsg.exec();
        }
        exit(1);
    }

    if(privileged)
        ((AS::QTNIXEngine*)as)->getIP()->loadQuests(QString("/etc/appset/")+QString(((AS::QTNIXEngine*)as)->getDistro().c_str())+
               QString("/")+QString(((AS::QTNIXEngine*)as)->getTool().c_str())+
               QString(".quest"));
#endif


    asThread = new AsThread(as);
    connect(asThread,SIGNAL(finished()),SLOT(opFinished()));
    connect(asThread,SIGNAL(userQuery(QString)),SLOT(userQuery(QString)));

    loadingDialog = new QDialog(this);
    loadingBar = new QProgressBar(loadingDialog);
    loadingStatus = new QLabel(loadingDialog);
    loadingDialog->setLayout(new QHBoxLayout());
    QLabel *lblLoading = new QLabel(loadingDialog);
    QWidget *rloading = new QWidget(loadingDialog);
    lblLoading->setPixmap(QPixmap(":/general/loading.png"));;
    lblLoading->setScaledContents(true);
    lblLoading->setFixedSize(loadingBar->height()*2,loadingBar->height()*2);
    loadingDialog->layout()->addWidget(lblLoading);
    loadingDialog->layout()->addWidget(rloading);
    rloading->setLayout(new QVBoxLayout());

    rloading->layout()->addWidget(loadingBar);
    rloading->layout()->addWidget(loadingStatus);

    flags = as_QUERY_ALL_INFO | as_EXPERT_QUERY;

    timer = new QTimer();
    timer2 = new QTimer();
    timerUpdate = new QTimer();
    connect(timerUpdate, SIGNAL(timeout()),SLOT(refresh()));
    timer->setSingleShot(true);
    connect(timer,SIGNAL(timeout()),SLOT(addRows()));    

    connect(timer2,SIGNAL(timeout()),SLOT(timeFilter()));

    timerConfirm = new QTimer();
    connect(timerConfirm,SIGNAL(timeout()),SLOT(confirmTimeout()));

    //ui->centralWidget->setDisabled(true);
    ui->mainToolBar->setDisabled(true);

    connect(ui->mainToolBar->addAction(style()->standardIcon(QStyle::SP_BrowserReload), tr("Update")), SIGNAL(triggered()), SLOT(getUpPrivileges()));
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
    connect(ui->mainToolBar->addAction(QIcon(":general/repos.png"), tr("Repositories")),SIGNAL(triggered()),SLOT(repoEditor()));

    ui->mainToolBar->addSeparator();

    connect(ui->mainToolBar->addAction(QIcon(":general/appset.png"), tr("About")),SIGNAL(triggered()),SLOT(about()));
    connect(ui->mainToolBar->addAction(QIcon(":general/bug.png"), tr("Report a bug")),SIGNAL(triggered()),SLOT(bugReport()));
    connect(ui->mainToolBar->addAction(QIcon(":general/feature.png"), tr("Request a feature")),SIGNAL(triggered()),SLOT(featureRequest()));

    connect(ui->searchBar, SIGNAL(textChanged(QString)), SLOT(timerFired(QString)));

    connect(ui->showUpgradable,SIGNAL(toggled(bool)),SLOT(showUpgradable(bool)));
    connect(ui->showInstalled,SIGNAL(toggled(bool)),SLOT(showInstalled(bool)));
    connect(ui->showNotInstalled,SIGNAL(toggled(bool)),SLOT(showNotInstalled(bool)));
    connect(ui->showAll,SIGNAL(toggled(bool)),SLOT(showAll(bool)));
    connect(ui->tableWidget,SIGNAL(cellClicked(int,int)),SLOT(changeStatus(int,int)));
    connect(ui->expertMode,SIGNAL(toggled(bool)),SLOT(expertMode(bool)));

    connect(ui->comboBox,SIGNAL(currentIndexChanged(int)),SLOT(searchTermChanged(int)));

    connect(ui->repoFilter,SIGNAL(currentIndexChanged(QString)),SLOT(repoFilter()));

    connect(ui->editCancel,SIGNAL(clicked()),SLOT(editCancel()));
    //connect(ui->editConfirm,SIGNAL(clicked()),SLOT(editConfirm()));
    connect(ui->editConfirm,SIGNAL(clicked()),SLOT(getPrivileges()));

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

    //ui->webView->installEventFilter(new QTEventFilter);

    //connect(ui->actionAbout_Qt,SIGNAL(triggered()),SLOT(aboutQt()));
    //connect(ui->actionAbout_AppSet,SIGNAL(triggered()),SLOT(about()));

    isExpert = false;

    /*loadingMovie = new QMovie(":/pkgstatus/loading.gif");
    loadingMovie->start();*/

    //RSS
    connect(ui->treeWidget, SIGNAL(itemPressed(QTreeWidgetItem*,int)),
            this, SLOT(itemActivated(QTreeWidgetItem*)));    
    QStringList headerLabels;
    headerLabels << tr("Link") << tr("Title");
    ui->treeWidget->setHeaderLabels(headerLabels);
    ui->treeWidget->header()->setResizeMode(QHeaderView::ResizeToContents);
    get(QUrl(QString(((AS::QTNIXEngine*)as)->getNewsUrl(QLocale::languageToString(QLocale::system().language()).toAscii().data()).c_str())));
    ui->treeWidget->hideColumn(0);
    rssloader=new QTimer(this);
    rssloader->setSingleShot(true);
    connect(rssloader,SIGNAL(timeout()),SLOT(RSSRetry()));

    mainSplitter = new QSplitter(ui->tabList);
    ui->contentsWidget->layout()->removeWidget(ui->extraInfoGroupBox);
    ui->contentsWidget->layout()->removeWidget(ui->tableWidget);
    ui->contentsWidget->layout()->addWidget(mainSplitter);
    mainSplitter->addWidget(ui->tableWidget);
    mainSplitter->addWidget(ui->extraInfoGroupBox);
    mainSplitter->setOrientation(Qt::Vertical);

    QList<int> sizes;
    sizes << 250 << 180;
    mainSplitter->setSizes(sizes);

    if(pp&&pp!=9&&pp!=11 && QFile::exists("/tmp/aspriv")){
        QFile file("/tmp/aspriv");
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream opf(&file);
        optPath = opf.readLine();
        QStringList geom=opf.readLine().split('\t');
        int x=geom.at(0).toInt()+(geom.at(2).toInt()-400)/2;
        int y=geom.at(1).toInt()+(geom.at(3).toInt()-300)/2;
        this->setGeometry(x,y,0,0);
    }else{
        optPath=QDir::homePath()+"/.appset-qt.conf";
    }
    Options opt(this,optPath);
    this->sbdelay=opt.sbdelay;
    this->extbrowser=opt.browser;
    this->showBackOut=opt.backOutput;
    this->confirmCountdown=opt.confirmCountdown;
    if(!opt.statShow) ui->statGroup->setHidden(true);
    if(opt.startfullscreen && (!pp || pp==9 || pp==11))this->maxShown=true;
    else this->maxShown=false;
    if(opt.showRepos)ui->tableWidget->showColumn(1);
    else ui->tableWidget->hideColumn(1);
    enhanced=opt.enhanced;
    if(enhanced)ui->urlpre->setHidden(true);
    ui->urlpre->setChecked(opt.extraInfo);
    this->autoupgrade=opt.autoupgrade;
    this->preload=opt.preload;
    this->interactions=opt.interactions;
    if(interactions==2)((AS::QTNIXEngine*)as)->setForced(true);
    else ((AS::QTNIXEngine*)as)->setForced(false);
    if(interactions==0 && !((AS::QTNIXEngine*)as)->isAuto()) ((AS::QTNIXEngine*)as)->setAuto(true);
    else if(interactions && ((AS::QTNIXEngine*)as)->isAuto()) ((AS::QTNIXEngine*)as)->setAuto(false);

    QFile helperDelay("/tmp/ashdelay.tmp");
    helperDelay.open(QIODevice::WriteOnly);
    if(helperDelay.isOpen()){
        QTextStream ashstream(&helperDelay);

        ashstream << opt.updelay;

        helperDelay.close();
    }

    connect(ui->infoText,SIGNAL(anchorClicked(QUrl)),SLOT(extBrowserLink(QUrl)));

    ui->backGroup->setHidden(true);

#ifdef unix
    ui->tabWidget->setTabText(2, QString(((AS::QTNIXEngine*)as)->getCommunityName().c_str()));

    QSplitter *splitter2 = new QSplitter(ui->tabCommunity);
    ui->contentsWidget->layout()->removeWidget(ui->extraInfoGroupBox_2);
    ui->contentsWidget->layout()->removeWidget(ui->groupCom);
    ui->tabCommunity->layout()->addWidget(splitter2);
    splitter2->addWidget(ui->groupCom);
    splitter2->addWidget(ui->extraInfoGroupBox_2);
    splitter2->setOrientation(Qt::Vertical);
    if(((AS::QTNIXEngine*)as)->isCommunityEnabled()){

        asComThread = new AsThread(as);
        connect(asComThread,SIGNAL(finished()),SLOT(comOpFinished()));        

        QStringList headers;
        headers << tr("S") << tr("Name") << tr("Installed Version") << tr("Last Version") << tr("Description");

        CommunityRepoModel *crm=new CommunityRepoModel(headers,0,(AS::QTNIXEngine*)as);
        ui->tableCommunity->setModel(crm);        

        ui->tableCommunity->setColumnWidth(0,24);


        QList<int> sizes;
        sizes << 250 << 180;
        splitter2->setSizes(sizes);
        connect(crm,SIGNAL(pkgInfoRetrieved(AS::Package*)),SLOT(comInfoRetrieved(AS::Package*)));

        connect(this,SIGNAL(installedPackagesUpdated(std::list<AS::Package*>*)),crm,SLOT(setInstalledPackages(std::list<AS::Package*>*)));

        connect(ui->tableCommunity,SIGNAL(clicked(QModelIndex)),SLOT(showMenu(QModelIndex)));

        timerComSearch = new QTimer();
        timerUpdateCom = new QTimer();
        //connect(timerComSearch,SIGNAL(timeout()),SLOT(comTimeFilter()));
        //connect(ui->lineEditCommunity,SIGNAL(textChanged(QString)),SLOT(comTimerFired(QString)));
        connect(this,SIGNAL(comPatternUpdated(QString)),crm,SLOT(setPattern(QString)));
        connect(crm,SIGNAL(dataUpdated()),SLOT(comTableUpdated()));

        connect(ui->tableCommunity->selectionModel(),SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),crm,SLOT(selectionChangedSlot(QModelIndex,QModelIndex)));

        connect(timerUpdateCom,SIGNAL(timeout()),SLOT(refreshCom()));

        ui->comContinue->setHidden(true);

        connect(ui->comContinueButton,SIGNAL(clicked()),qApp,SLOT(quit()));
        connect(ui->lineEditCommunity,SIGNAL(returnPressed()),SLOT(comTimeFilter()));
        connect(ui->comSearch,SIGNAL(clicked()),SLOT(comTimeFilter()));
        connect(ui->comClear,SIGNAL(clicked()),SLOT(clearComLine()));
        connect(ui->comUpgrade,SIGNAL(clicked()),SLOT(upgradeCom()));

        connect(crm,SIGNAL(dataUpdated()),ui->Searching,SLOT(hide()));
        connect(ui->comInstalled,SIGNAL(clicked()),crm,SLOT(listInstalled()));
    }else{
        //ui->tabWidget->removeTab(2);
        splitter2->setHidden(true);
        ui->comTools->setHidden(true);
        QLabel *com_support=new QLabel(tr("To enable external packages support you have to install")+QString(" ")+
                                      QString(((AS::QTNIXEngine*)as)->getCommunityToolName().c_str()),ui->tabCommunity);
        com_support->setStyleSheet("color: rgb(170, 0, 0);font: 75 italic 14pt \"Sans Serif\";");
        ui->tabCommunity->layout()->addWidget(com_support);
        if(((AS::QTNIXEngine*)as)->getCommunityToolName().find("*")!=std::string::npos)
                ui->tabWidget->removeTab(2);
    }    

    oldgeom=QRect(-1,0,0,0);

    if((pp!=9 && pp!=5 && pp!=10 && pp) || pp==11){
        this->showPriv();
        timer->start(100);
    }else{
        this->hidePriv();
        oldgeom=QRect(-1,0,0,0);

        if(preload) timer->start(100);
    }
#endif

    inModal=false;

    priv = new QProcess(this);

    //QWebSettings::setIconDatabasePath("/tmp/favicons/");
    connect(ui->webView,SIGNAL(iconChanged()),SLOT(appIcon()));    
}


void MainWindow::appIcon(){
    ;//ui->tableWidget->item(currentPacket,2)->setIcon(ui->webView->icon().pixmap(16,16));
}

void MainWindow::cleanCache(){
#ifdef unix
    QMessageBox con;
    con.setText(tr("Are you sure to clean the cache?"));
    con.setIcon(QMessageBox::Question);
    con.setStandardButtons(QMessageBox::Yes|QMessageBox::No);

    if(con.exec()==QMessageBox::Yes){
        this->setEnabled(false);

        QFile pfile("/tmp/aspriv");
        pfile.open(QIODevice::WriteOnly|QIODevice::Text);
        QTextStream aspriv(&pfile);
        aspriv << QDir::homePath() << "/.appset-qt.conf\n";
        aspriv << this->x() << "\t" << this->y() << "\t" << this->width() << "\t" << this->height() << "\n";
        pfile.close();

        priv = new QProcess(this);
        QStringList args;
        args << "--cleancache";

        priv->start(QString("appset-launch.sh"),args);

        connect(priv,SIGNAL(finished(int)),SLOT(outCCachePrivileged(int)));
    }
#endif
}

void MainWindow::repoEditor(){
#ifdef unix
    system((QString("appset-launch.sh --repoedit /etc/appset/")+QString(((AS::QTNIXEngine*)as)->getDistro().c_str())+
           QString("/")+QString(((AS::QTNIXEngine*)as)->getTool().c_str())+
           QString(".repos &")).toAscii().data());
#endif
}

void MainWindow::outCCachePrivileged(int out){
    int cacheSize = ((QTNIXEngine*)as)->cacheSize();
    cleanAction->setEnabled(cacheSize>0);
    if(cacheSize>0){
        cleanAction->setText(tr("Clean cache")+QString(" (")+QString::number(cacheSize)+QString(" MB)"));
    }else{
        cleanAction->setText(tr("Clean cache"));
    }

    this->setEnabled(true);

    priv->disconnect(SIGNAL(finished(int)),this,SLOT(outCCachePrivileged(int)));
    QFile::remove("/tmp/aspriv");
}

void MainWindow::getUpPrivileges(){
    this->setEnabled(false);

    QFile pfile("/tmp/aspriv");
    pfile.open(QIODevice::WriteOnly|QIODevice::Text);
    QTextStream aspriv(&pfile);
    aspriv << QDir::homePath() << "/.appset-qt.conf\n";
    aspriv << this->x() << "\t" << this->y() << "\t" << this->width() << "\t" << this->height() << "\n";
    pfile.close();

    priv = new QProcess(this);
    QStringList args;
    args << "--update";

    priv->start(QString("appset-launch.sh"),args);

    connect(priv,SIGNAL(finished(int)),SLOT(outUpPrivileged(int)));
}

void MainWindow::getPrivileges(){
    int rows=ui->tableUpgraded->rowCount();

    QFile lfile("/tmp/aslock");
    lfile.open(QIODevice::WriteOnly);
    lfile.close();

    QFile pfile("/tmp/aspriv");
    pfile.open(QIODevice::WriteOnly|QIODevice::Text);
    QTextStream aspriv(&pfile);
    aspriv << QDir::homePath() << "/.appset-qt.conf\n";
    aspriv << this->x() << "\t" << this->y() << "\t" << this->width() << "\t" << this->height() << "\n";
    pfile.close();

    QFile ifile(local?"/tmp/aslocal":"/tmp/asinstall");
    ifile.open(QIODevice::WriteOnly|QIODevice::Text);
    QTextStream asfiler;

    QFile ufile("/tmp/asupgrade");
    ufile.open(QIODevice::WriteOnly|QIODevice::Text);

    QFile rfile("/tmp/asremove");
    rfile.open(QIODevice::WriteOnly|QIODevice::Text);
    for(int i=0;i<rows;++i){
        if(ui->tableUpgraded->item(i,0)->text()=="Remove"){
            asfiler.setDevice(&rfile);
        }else if(ui->tableUpgraded->item(i,0)->text()=="Upgrade" || (ui->tableUpgraded->item(i,0)->text()=="Upgradable" && toU)){
            asfiler.setDevice(&ufile);
        }else{
            asfiler.setDevice(&ifile);
        }
        asfiler << ui->tableUpgraded->item(i,1)->text() << "\t-\t" << ui->tableUpgraded->item(i,0)->text() << "\t-\t";
        asfiler << ui->tableUpgraded->item(i,2)->text() << "\t-\t" << ui->tableUpgraded->item(i,3)->text() << "\t-\t";
        asfiler << ui->tableUpgraded->item(i,4)->text() << (((ui->tableUpgraded->item(i,0)->text()=="Upgradable")?QString("\t-\tH"):QString(""))+"\n");
    }
    ifile.close();
    rfile.close();
    ufile.close();

    int ii=toI,rr=toR,uu=toU;
    editConfirm();
    toR=0;
    editConfirm();
    toI=0;
    editConfirm();
    toU=0;
    this->setEnabled(false);

    priv = new QProcess(this);
    QStringList args;
    if(local){
        args << "--local";
    }else{
        if(ii)args << "--install";
        if(uu)args << "--upgrade";
        if(rr)args << "--remove";
    }
    priv->start(QString("appset-launch.sh"),args);

    connect(priv,SIGNAL(finished(int)),SLOT(outPrivileged(int)));
}

void MainWindow::getComPrivileges(){
    int ii=toI,rr=toR,uu=toU;

    QFile lfile("/tmp/aslock");
    lfile.open(QIODevice::WriteOnly);
    lfile.close();

    QFile pfile("/tmp/aspriv");
    pfile.open(QIODevice::WriteOnly|QIODevice::Text);
    QTextStream aspriv(&pfile);
    aspriv << QDir::homePath() << "/.appset-qt.conf\n";
    aspriv << this->x() << "\t" << this->y() << "\t" << this->width() << "\t" << this->height() << "\n";
    pfile.close();

    QFile ifile("/tmp/ascom");
    ifile.open(QIODevice::WriteOnly|QIODevice::Text);
    QTextStream asfiler(&ifile);
    asfiler << comPattern << "\n";
    ifile.close();

    this->setEnabled(false);

    QStringList args;
    if(ii)args << "--cominstall";
    if(uu)args << "--comupgrade";
    if(rr)args << "--comremove";
    priv->start(QString("appset-launch.sh"),args);

    connect(priv,SIGNAL(finished(int)),SLOT(outComPrivileged(int)));
}

void MainWindow::outUpPrivileged(int out){
    this->setEnabled(true);

    addRows(true);

    QFile::remove("/tmp/aspriv");
    priv->disconnect(SIGNAL(finished(int)),this,SLOT(outUpPrivileged(int)));
}

void MainWindow::outComPrivileged(int out){
    this->setEnabled(true);

    refreshCom();
    comContinued();

    QFile::remove("/tmp/ascom");
    QFile::remove("/tmp/aspriv");
    priv->disconnect(SIGNAL(finished(int)),this,SLOT(outComPrivileged(int)));
}

void MainWindow::outPrivileged(int out){
    this->setEnabled(true);
    opFinished();

    QFile::remove("/tmp/asinstall");
    QFile::remove("/tmp/asupgrade");
    QFile::remove("/tmp/asremove");
    QFile::remove("/tmp/aspriv");

    priv->disconnect(SIGNAL(finished(int)),this,SLOT(outPrivileged(int)));
}

int MainWindow::argInterpreter(QString arg){
    if(arg=="--install") return 1;
    if(arg=="--remove") return 2;
    if(arg=="--upgrade") return 3;
    if(arg=="--local") return 4;
    if(arg=="--cominstall") return 6;
    if(arg=="--comremove") return 7;
    if(arg=="--comupgrade") return 8;
    if(arg=="--update") return 5;
    if(arg=="--hidden") return 9;
    if(arg=="--cleancache") return 10;
    if(arg=="--show") return 11;
    return 0;
}

int MainWindow::privilegedExecuter(int argc, char *argv[]){
    if(argc<2)return 0;
    int actual = 1, i=0; QString opString;
    toR=toU=toI=0;

    while(argc>actual){
        opString=argv[actual];
        int op=argInterpreter(opString);
        if(op>=1 && op<=4){
            if(op==4)local=true;

            opString.remove('-');
            ui->stacked->setCurrentIndex(1);

            ui->tableUpgraded->hideColumn(2);
            ui->tableUpgraded->hideColumn(3);
            ui->tableUpgraded->hideColumn(4);
            ui->mainToolBar->hide();
            ui->choicesGroup->hide();

            ui->line->hide();
            ui->label_5->hide();
            ui->tableCommunity->setColumnWidth(1,250);
            this->setFixedWidth(550);
            this->setFixedHeight(400);

            QFile file(QString("/tmp/as")+opString);
            file.open(QIODevice::ReadOnly | QIODevice::Text);
            QTextStream packages(&file);
            QString line = packages.readLine();

            while(!line.isNull()){
                if(op==1 || op==4)toI++;
                else if(op==2)toR++;
                else if(op==3)toU++;

                QStringList args=line.split("\t-\t");
                ui->tableUpgraded->insertRow(i);
                ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(QString(":/pkgstatus/")+opString+".png"),args.at(1)));
                ui->tableUpgraded->setItem(i,1,new QTableWidgetItem(args.at(0)));
                ui->tableUpgraded->setItem(i,2,new QTableWidgetItem(args.at(2)));
                ui->tableUpgraded->setItem(i,3,new QTableWidgetItem(args.at(3)));
                ui->tableUpgraded->setItem(i,4,new QTableWidgetItem(args.at(4)));
                if(args.count()>5){
                    ui->tableUpgraded->hideRow(i);
                }
                QProgressBar *prog=new QProgressBar();
                prog->setValue(0);
                ui->tableUpgraded->setCellWidget(i,5,prog);
                i++;

                line = packages.readLine();
            }

            file.close();
        }else if(op>=6 && op<=8){
            ui->stacked->setCurrentIndex(2);

            QFile ifile("/tmp/ascom");
            ifile.open(QIODevice::ReadOnly|QIODevice::Text);
            QTextStream asfiler(&ifile);
            comPattern=asfiler.readLine().trimmed();

            this->setFixedWidth(700);
            this->setFixedHeight(400);

            return op;
        }else return op;

        actual++;
    }    

    return 1;
}

void MainWindow::cancelOps(){
    QMessageBox quest;
    quest.setText(tr("Are you sure to stop current operations?"));
    quest.setIcon(QMessageBox::Question);
    quest.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
    int answer = quest.exec();

    if(answer==QMessageBox::Yes){
        //Confirmed to stop operations
        ((AS::QTNIXEngine*)as)->requestStop();
    }
}

void MainWindow::hideEvent(QHideEvent *e){
    loadingDialog->hide();
}

void MainWindow::clearComLine(){
    ui->lineEditCommunity->clear();
    comTimeFilter();
}

void MainWindow::comContinued(){
    ui->comContinue->setHidden(true);
    ui->mainToolBar->setVisible(true);
    ui->comWait->setVisible(true);
    ui->stacked->setCurrentIndex(0);

    merging=false;
    addRows();

    //ui->lineEditCommunity->setText("");
}

void MainWindow::comInfoRetrieved(AS::Package *pkg){
    if(pkg) ui->webView_2->setUrl(QUrl(pkg->getURL().c_str()));
}

#include <QScrollBar>
void MainWindow::refreshCom(){
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
            if(logs.size()>60)logs.removeFirst();
        }        

        if(logs.size()) ui->backOut_2->setText(logs.join("\n"));


        QScrollBar *bar = ui->backOut_2->verticalScrollBar();
        bar->setValue(bar->maximum());
    }
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
        ui->tableWidget->setItem(0,2, new QTableWidgetItem((*(pkgs->rbegin()))->getName().c_str()));
        ui->tableWidget->setItem(0,4, new QTableWidgetItem(fileName));
        ui->tableWidget->setItem(0,7,new QTableWidgetItem(QString((*(pkgs->rbegin()))->getRemoteVersion().c_str())));

        delete pkgs;

        local=true;
        confirm();
    }else{
        QMessageBox error;
        error.setText(tr("Error loading the specified package file!"));
        error.setInformativeText(tr("The file")+QString(" \"")+QString(fileName)+QString("\" ")+tr("doesn't seems to be a valid package!"));
        error.setIcon(QMessageBox::Critical);
        error.setStandardButtons(QMessageBox::Ok);
        inModal=true;
        error.exec();
        inModal=false;

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

void MainWindow::saveUrlPre(){
    Options opt(this,optPath);
    opt.extraInfo=ui->urlpre->isChecked();
    opt.writeConfigFile(true,true);
}

void MainWindow::showOptions(){
    if(!this->isVisible())return;

    Options opt(this,QDir::homePath()+"/.appset-qt.conf");
    inModal=true;
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
        enhanced=opt.enhanced;
        mainSplitter->setHidden(enhanced);
        this->autoupgrade=opt.autoupgrade;
        this->preload=opt.preload;
        this->interactions=opt.interactions;
        if(interactions==2)((AS::QTNIXEngine*)as)->setForced(true);
        else ((AS::QTNIXEngine*)as)->setForced(false);
        if(interactions==0 && !((AS::QTNIXEngine*)as)->isAuto()) ((AS::QTNIXEngine*)as)->setAuto(true);
        else if(interactions && ((AS::QTNIXEngine*)as)->isAuto()) ((AS::QTNIXEngine*)as)->setAuto(false);

        QFile helperDelay("/tmp/ashdelay.tmp");
        helperDelay.open(QIODevice::WriteOnly);
        if(helperDelay.isOpen()){
            QTextStream ashstream(&helperDelay);

            ashstream << opt.updelay << "\n";

            helperDelay.close();
        }

        if(enhanced && !view)asyncFilter();
        if(!enhanced && view){
            delete view;
            view=0;
        }

        ui->urlpre->setHidden(enhanced);

        if(view)view->setVisible(enhanced);
    }

    inModal=false;
}

#include <QWebSettings>

void MainWindow::hidePriv(){
    if(!inModal){
        QFile::remove("/tmp/asshown");
        oldgeom=geometry();
        hide();

        if(!preload) clearPackagesList();

        QWebSettings::clearMemoryCaches();
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
    ui->textBrowser->page()->setPalette(QPalette(Qt::white));
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
    rssloader->start(4000);
    //ui->newsGroup->hide();
}

void MainWindow::RSSRetry(){
    get(QUrl(QString(((AS::QTNIXEngine*)as)->getNewsUrl(QLocale::languageToString(QLocale::system().language()).toAscii().data()).c_str())));
}

//END RSS

void MainWindow::confirmTimeout(){
    confirmRemaining--;

    if(confirmRemaining==0){
        timerConfirm->stop();

        ui->editConfirm->setText(tr("Confirm"));

        //editConfirm();
        getPrivileges();
    }else ui->editConfirm->setText(tr("Confirm")+QString(" (")+QString::number(confirmRemaining)+QString(")"));
}

void MainWindow::aboutQt(){
    QMessageBox::aboutQt(this);
}
#include "about.h"
void MainWindow::about(){
    inModal=true;
    ab.exec();
    inModal=false;
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
    bool scrolled=false;
    int currents = 0;
    switch(asThread->getOp()){
    case 1: //Install
        rows=ui->tableUpgraded->rowCount();
        for(int i=0;i<rows;++i){
            if(ui->tableUpgraded->item(i,0)->text()=="Upgrade" || ui->tableUpgraded->item(i,0)->text()=="Remove" || completed[i+baseIndex]) continue;
            else currents++;

            Package pkg;
            QStringList ap=ui->tableUpgraded->item(i,1)->text().split('/');
            QString pp;
            if(ap.size()>1){
                pp = (ap.at(1));
            }else{
                pp = (ap.at(0));
            }
            pkg.setName(pp.toAscii().data());

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

            if(perc>0 && !scrolled){
                ui->tableUpgraded->scrollToItem(ui->tableUpgraded->item(i,0));
                scrolled=true;
            }

            if(perc>=99){
                ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setValue(100);
                ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setFormat(tr("Waiting others..."));
                ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/working.png"),ui->tableUpgraded->item(i,0)->text()));

                bool totalCompleted=true;
                for(int k=0;totalCompleted && k<currents;++k){
                    if(!completed[k]) totalCompleted=false;
                }

                if(totalCompleted){
                    ui->cancelOps->setDisabled(true);
                    for(int k=0;k<rows;++k){
                        ((QProgressBar*)ui->tableUpgraded->cellWidget(k,5))->setFormat(tr("Installing..."));
                    }

                    break;
                }
                else if(asThread->getOp()==1) ui->cancelOps->setEnabled(true);
            }
        }
        break;
     case 2:
        rows=ui->tableUpgraded->rowCount();
        for(int i=0;i<rows;++i){
            if(ui->tableUpgraded->item(i,0)->text()!=QString("Upgrade") || completed[i+baseIndex]) continue;
            else currents++;

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

            if(perc>0 && !scrolled){
                ui->tableUpgraded->scrollToItem(ui->tableUpgraded->item(i,0));
                scrolled=true;
            }

            if(perc>=99){
                completed[i+baseIndex]=true;
                ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setValue(100);
                ((QProgressBar*)ui->tableUpgraded->cellWidget(i,5))->setFormat(tr("Waiting others..."));
                ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/working.png"),"Upgrade"));


                bool totalCompleted=true;
                for(int k=0;totalCompleted && k<currents;++k){
                    if(!completed[k+baseIndex]) totalCompleted=false;
                }

                if(totalCompleted){
                    ui->cancelOps->setDisabled(true);
                    for(int k=0;k<rows;++k){
                        /*QLabel *label = new QLabel();
                        label->setMovie(loadingMovie);
                        ui->tableUpgraded->setCellWidget(k,0,label);*/
                        ((QProgressBar*)ui->tableUpgraded->cellWidget(k,5))->setFormat(tr("Installing..."));
                    }

                    break;
                }else if(asThread->getOp()==2) ui->cancelOps->setEnabled(true);
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
    int op = 1;
    if(pp!=9 && pp && pp!=11){
        timerUpdate->stop();
        op=asThread->getOp();
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
                    if(ui->tableUpgraded->item(i,0)->text()=="Upgrade" || ui->tableUpgraded->item(i,0)->text()==QString("Remove") || ui->tableUpgraded->item(i,0)->text()=="Upgradable") continue;
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
                    if(ui->tableUpgraded->item(i,0)->text()=="Upgrade"){
                        if(ui->tableUpgraded->item(i,1)->text()==QString(((AS::QTNIXEngine*)as)->getTool().c_str())){
                            loadingStatus->setText(tr("Running backend's post upgrade command"));
                            loadingBar->show();
                            loadingBar->setValue(50);
                            ((AS::QTNIXEngine*)as)->toolUpgrade();
                            loadingStatus->setText("");
                            loadingBar->hide();
                            loadingBar->setValue(0);
                        }

                        ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/success.png"),"Upgrade"));
                    }
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

    }

    if(op>0&&op<4){
        if(pp==9 || !pp || pp==11){
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
                inModal=true;
                int showLogs = this->isVisible()?!QFile::exists("/tmp/aslock")?done.exec():QMessageBox::No:QMessageBox::No;
                inModal=false;

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

                    inModal=true;
                    logDialog.exec();
                    inModal=false;

                    disconnect(&logDialog);
                }
            }else ui->statusBar->showMessage(tr("All operations completed successfully!"),5000);//done.show();
        }else if((!toI && !toU && !toR)){
            as->removeListener(logger);
            delete logger;
            qApp->quit();
        }else{
            editConfirm();
        }
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

        if(pp!=9 && pp && pp!=11){
            asThread->setList(prem);
            asThread->setOp(3);
            pkgs=prem;
            asThread->start();
        }
    }else if(toI){
        std::list<Package*> *pinst=new std::list<Package*>();
        for(int i=0;i<rows;++i){
            if(ui->tableUpgraded->item(i,0)->text()==QString("Remove") || ui->tableUpgraded->item(i,0)->text()=="Upgrade" || ui->tableUpgraded->item(i,0)->text()=="Upgradable") continue;
            p=new Package();
            if(!local)p->setName(ui->tableUpgraded->item(i,1)->text().trimmed().toAscii().data());
            else p->setName(ui->tableUpgraded->item(i,2)->text().trimmed().toAscii().data());
            pinst->insert(pinst->end(), p);
            ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/waiting.png"),ui->tableUpgraded->item(i,0)->text()));

            iOutcome << ui->tableUpgraded->item(i,1)->toolTip().trimmed();
        }

        if(pp!=9 && pp && pp!=11){
            asThread->setList(pinst);
            asThread->setOp(1);
            asThread->local=local;
            pkgs=pinst;
            asThread->start();
        }
    }else if(toU){
        std::list<Package*> *pupgr=new std::list<Package*>();

        bool ignoring = ((AS::QTNIXEngine*)as)->isIgnoringUpgrades();
        if(ignoring){
            int rows = ui->tableUpgraded->rowCount();
            for(int i=0;i<rows;++i){                
                if(ui->tableUpgraded->item(i,0)->text()=="Upgradable"){
                    p=new Package();
                    p->setName(ui->tableUpgraded->item(i,1)->text().trimmed().toAscii().data());
                    pupgr->insert(pupgr->end(),p);
                }else if(ui->tableUpgraded->item(i,0)->text()=="Upgrade"){
                    uOutcome << ui->tableUpgraded->item(i,1)->toolTip().trimmed();
                }
            }
        }else{
            int rows = ui->tableUpgraded->rowCount();
            for(int i=0;i<rows;++i){
                if(ui->tableUpgraded->item(i,0)->text()=="Upgrade"){
                    ui->tableUpgraded->setItem(i,0,new QTableWidgetItem(QIcon(":pkgstatus/waiting.png"),"Upgrade"));
                    p=new Package();
                    p->setName(ui->tableUpgraded->item(i,1)->text().trimmed().toAscii().data());
                    pupgr->insert(pupgr->end(),p);

                    uOutcome << ui->tableUpgraded->item(i,1)->toolTip().trimmed();
                }
            }
        }

        //XXX need to check for replacements

        if(pp!=9 && pp && pp!=11){
            asThread->setList(pupgr);
            asThread->setOp(2);
            pkgs=pupgr;
            asThread->start();
        }
    }

    if(toU || toR || toI){        
        ui->choicesGroup->setHidden(true);

        if(pp!=9 && pp && pp!=11){
            timerUpdate->start(250);
            if(this->showBackOut) ui->backGroup->setVisible(true);
        }
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

void MainWindow::comTimeFilter(){
    emit searching();
    ui->Searching->setVisible(true);
    QCoreApplication::processEvents(QEventLoop::AllEvents,500);
    emit comPatternUpdated(ui->lineEditCommunity->text());    
}

void MainWindow::comTableUpdated(){
    if(ui->tableCommunity->model()->rowCount()){
        ui->tableCommunity->selectRow(0);
    }
}

void MainWindow::comTimerFired(QString s){
    timerComSearch->stop();
    timerComSearch->setSingleShot(true);
    timerComSearch->start(this->sbdelay);s="";
}

void MainWindow::editCancel(){
    timerConfirm->stop();

    ui->stacked->setCurrentIndex(0);
    ui->mainToolBar->show();

    if(local)addRows();
}

void MainWindow::markUpgrades(){
    loadingDialog->show();
    loadingBar->setValue(0);
    int rows = ui->tableWidget->rowCount();
    int count=0;
    for(int i=0;i<rows;++i){
        if(ui->tableWidget->item(i,0)->text()=="Upgradable"){            
            currentPacket=i;
            upgrade(true);

            QCoreApplication::processEvents(QEventLoop::AllEvents, 33);
            count++;
            loadingBar->setValue(count*100/(upgradables?upgradables:count));
        }
    }
    loadingBar->setValue(0);
    loadingDialog->hide();

    if(enhanced){
        for(int i=0;i<appsList.count();++i){
            if(((AppItem*)appsList.at(i))->status()=="Upgradable")
                ((AppItem*)appsList.at(i))->setStatus("Upgrade");
        }

        asyncFilter();
    }
}

void MainWindow::searchTermChanged(int x){
    asyncFilter(ui->searchBar->text());x=0;
}
#include <QCompleter>
void MainWindow::asyncFilter(QString filter){
    timer2->stop();
    int rows = ui->tableWidget->rowCount();

    if(filter=="@") filter = ui->searchBar->text();
    else if(filter=="@@"){ filter = ui->searchBar->text(); filterPressed(); return;}
    else{ filterPressed(); return;}

    filter=filter.trimmed();
    filter.replace(' ','|');

    int index = ui->comboBox->currentIndex();

    if(this->isVisible())loadingDialog->show();

    ui->centralWidget->setEnabled(false);

    if(index==2){
        for(int i=0;i<rows;++i){
            if(!ui->tableWidget->item(i,2) || !ui->tableWidget->item(i,5) || (!isExpert && ui->tableWidget->item(i,2)->text().contains(expert)) ||
               (ui->tableWidget->item(i,2)->text().contains(category_exclude) || ui->tableWidget->item(i,5)->text().contains(category_exclude))
                || isRepoFiltered(ui->tableWidget->item(i,1)->text())
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
                || isRepoFiltered(ui->tableWidget->item(i,1)->text())
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

    if(!enhanced){
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

    loadingBar->setValue(0);

    //XXX Need better completer
    /*QStringList pnames;
    for(int i=0;i<rows;++i){
        if(!(ui->tableWidget->isRowHidden(i)))
            pnames << ui->tableWidget->item(i,2)->text();
    }
    if(pcomp)delete pcomp;
    pcomp=new QCompleter(pnames,ui->searchBar);
    pcomp->setCaseSensitivity(Qt::CaseInsensitive);
    ui->searchBar->setCompleter(pcomp);*/

    if(enhanced){
        if(view)QMetaObject::invokeMethod(view->rootObject(),"closeDetails");
        appsList.clear();
        for(int i=0;i<rows;++i){
            if(ui->tableWidget->isRowHidden(i))continue;

            AppItem *app = new AppItem();

            app->setName(ui->tableWidget->item(i,2)->text());
            app->setAppUrl(ui->tableWidget->item(i,6)->text());
            app->setDescription(ui->tableWidget->item(i,5)->text());
            app->setLVersion(ui->tableWidget->item(i,3)->text());
            app->setRVersion(ui->tableWidget->item(i,4)->text());
            app->setRepo(ui->tableWidget->item(i,1)->text());
            app->setStatus(ui->tableWidget->item(i,0)->text());
            app->setI(i);
            app->setDSize(ui->tableWidget->item(i,7)->text().toInt());

            app->setLVersionStr(tr("Installed Version"));
            app->setRVersionStr(tr("Last Version"));
            app->setDSizeStr(tr("Size"));
            app->setRepoStr(tr("Repository"));
            app->setDeps(app->status()=="Installed"||app->status()=="Remove"?tr("Required by"):tr("Requires"));
            app->setCloseStr(tr("Close"));
            app->setInstallStr(tr("Install"));
            app->setUpdateStr(tr("Upgrade"));
            app->setRemoveStr(tr("Remove"));

            appsList.append(app);
        }
        if(!view){
            view=new QDeclarativeView;
            view->rootContext()->setContextProperty("appsModel",QVariant::fromValue(appsList));
            view->rootContext()->setContextProperty("appset",this);
            view->setSource(QUrl("qrc:/appsView/AppsView.qml"));
            view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
            mainSplitter->setHidden(true);
            ui->contentsWidget->layout()->addWidget(view);
        }else{
            view->rootContext()->setContextProperty("appsModel",QVariant::fromValue(appsList));
        }
    }

}

void MainWindow::installWheel(QObject *ob){
    QTEventFilter *filter=new QTEventFilter;
    ob->installEventFilter(filter);
    connect(filter,SIGNAL(wheel(int)),this,SLOT(wheel(int)));
}

void MainWindow::wheel(int delta){
    if(view)QMetaObject::invokeMethod(view->rootObject(),"wheel",Q_ARG(QVariant,delta));
}

void MainWindow::resizeEvent(QResizeEvent *e){
    if(view)QMetaObject::invokeMethod(view->rootObject(),"closeDetails");
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

void MainWindow::installCom(){
    if(!pp || pp==9 || pp==11){
        toI=1;toU=0;toR=0;
        comPattern=ui->tableCommunity->model()->data(ui->tableCommunity->model()->index(currentPacket,1)).toString();
        getComPrivileges();
    }else{
        comCommon(6);
    }
}

void MainWindow::upgradeCom(){
    QMessageBox con;
    con.setText(tr("Are you sure to upgrade ALL external packages?"));
    con.setIcon(QMessageBox::Question);
    con.setStandardButtons(QMessageBox::Yes|QMessageBox::No);

    inModal=true;
    if(con.exec()==QMessageBox::Yes){
        //comCommon(8);
        toI=0;toU=1;toR=0;
        getComPrivileges();
    }
    inModal=false;
}

void MainWindow::removeCom(){
    if(!pp || pp==9 || pp==11){
        toI=0;toU=0;toR=1;
        comPattern=ui->tableCommunity->model()->data(ui->tableCommunity->model()->index(currentPacket,1)).toString();
        getComPrivileges();
    }else{
        comCommon(7);
    }
}

void MainWindow::comCommon(int op){
    ui->stacked->setCurrentIndex(2);

    logger=new ASLogger();
    as->addListener(logger);

    timerUpdateCom->start(150);

    ui->mainToolBar->setHidden(true);
    ui->comWait->setVisible(true);

    asComThread->setOp(op);
    if(op!=8)asComThread->pattern=this->comPattern;
    asComThread->start();
}

void MainWindow::comOpFinished(){
    timerUpdateCom->stop();
    //as->removeListener(logger);
    //delete logger;
    //refreshCom();
    ui->comContinue->setVisible(true);
    ui->comWait->setHidden(true);
}

void MainWindow::install(bool community){
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
                        if(!instaDeps.contains(name,pname)){
                            currentPacket=i;
                            instaDeps.insert(name, pname);
                            this->install(true);
                        }
                    }
                }
            }
            delete(*it);
        }
        delete pkgs;
    }

    applyEnabler();
    ui->statusBar->showMessage(tr("Pending changes:")+QString::number(modified),2000);

    if(enhanced && !community)asyncFilter();
}

void MainWindow::remove(bool community){
    Package p;
    QStringList req;
    p.setName(ui->tableWidget->item(currentPacket,2)->text().trimmed().toAscii().data());
    QString pname(p.getName().c_str());
    if((pkgs = as->checkDeps(&p,false))){
        for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
            QString name = QString((*it)->getName().c_str()).trimmed();            
            if(name!=pname && !remDeps.contains(name))
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
        inModal=true;
        res = reqMes.exec();
        inModal=false;
    }

    if(res == QMessageBox::Yes){
        ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(QIcon(":pkgstatus/remove.png"),"Remove"));
        modified++;

        //Select deps too
        for(int j=0;j<req.size();++j){
            QString cur=req.at(j);
            for(int i=0;i<ui->tableWidget->rowCount();++i){
                if(ui->tableWidget->item(i,2)->text()==cur){                    
                    if(!remDeps.contains(cur,pname)){
                        currentPacket=i;
                        QString status=ui->tableWidget->item(i,0)->text();
                        if(status=="Remove") continue;
                        remDeps.insert(cur, pname);
                        this->remove(true);
                    }
                }
            }
        }

        applyEnabler();
    }

    ui->statusBar->showMessage(tr("Pending changes:")+QString::number(modified),2000);

    if(enhanced && !community)asyncFilter();
}

void MainWindow::upgrade(bool community){
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
                        QString status=ui->tableWidget->item(i,0)->text();
                        if(status=="Upgrade" || status=="Install") continue;
                        if(status=="Upgradable") this->upgrade(true);
                        else this->install(true);
                    }
                }
            }
            delete(*it);
        }
        delete pkgs;
    }


    applyEnabler();

    ui->statusBar->showMessage(tr("Pending changes:")+QString::number(modified),2000);

    if(enhanced && !community)asyncFilter();
}

void MainWindow::notInstall(bool community){
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
        inModal=true;
        res = reqMes.exec();
        inModal=false;
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
            this->notInstall(true);
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
            if(isExpert){
                inModal=true;
                resp=reqMes.exec();
                inModal=false;
            }
            for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                QString name = QString((*it)->getRepository().c_str()).trimmed()+QString("/")+QString((*it)->getName().c_str()).trimmed();
                if(name!=dname){
                    for(int i=0;i<ui->tableWidget->rowCount();++i){
                        if(ui->tableWidget->item(i,2)->toolTip()==name){
                            instaDeps.remove(name,dname);
                            if(!isExpert || resp == QMessageBox::Yes){
                                currentPacket = i;
                                this->notInstall(true);
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

    if(enhanced && !community)asyncFilter();
}

void MainWindow::notRemove(bool community){
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
        inModal=true;
        res = reqMes.exec();
        inModal=false;
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
            this->notRemove(true);
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
            if(isExpert){
                inModal=true;
                resp=reqMes.exec();
                inModal=false;
            }
            for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                QString name = QString((*it)->getName().c_str()).trimmed();
                if(name!=dname){
                    for(int i=0;i<ui->tableWidget->rowCount();++i){
                        if(ui->tableWidget->item(i,2)->text()==name){
                            remDeps.remove(name,dname);
                            if(!isExpert || resp == QMessageBox::Yes){
                                currentPacket = i;
                                this->notRemove(true);
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

    if(enhanced && !community)asyncFilter();
}

void MainWindow::notUpgrade(bool community){
    ui->tableWidget->setItem(currentPacket,0,new QTableWidgetItem(QIcon(":pkgstatus/upgrade.png"),"Upgradable"));
    modified--;
    applyEnabler();
    ui->statusBar->showMessage(tr("Pending changes:")+QString::number(modified),2000);

    if(enhanced && !community)asyncFilter();
}

void MainWindow::confirm(){
    ui->tableUpgraded->hideColumn(5);
    ui->tableUpgraded->setColumnWidth(1,300);
    ui->tableUpgraded->setColumnWidth(2,200);
    ui->tableUpgraded->setColumnWidth(3,300);
    ui->stacked->setCurrentIndex(1);
    ui->mainToolBar->hide();

    ui->tableUpgraded->clearContents();
    ui->tableUpgraded->setRowCount(0);

    int in=0,r=0,u=0;
    bool ignoring = ((AS::QTNIXEngine*)as)->isIgnoringUpgrades();
    int rows=ui->tableWidget->rowCount();
    for(int i=0;i<rows;++i){
        if(autoupgrade && !isExpert && ui->tableWidget->item(i,0)->text()=="Upgradable" && ui->tableWidget->item(i,2)->text().contains(expert)){
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

                    ui->tableUpgraded->setItem(r,3,new QTableWidgetItem(deps.join(" ")));
                    ui->tableUpgraded->setItem(r,4,new QTableWidgetItem(QString::number(ui->tableWidget->item(i,7)->text().toFloat()/1024)));

                    ui->tableUpgraded->setItem(r++,2,new QTableWidgetItem(*ui->tableWidget->item(i,3)));
        }else if(ui->tableWidget->item(i,0)->text()=="Install" && (!ui->tableWidget->item(i,2) || instaDeps.value(ui->tableWidget->item(i,2)->toolTip())=="")){
            ui->tableUpgraded->insertRow(in);
            ui->tableUpgraded->setItem(in,0,new QTableWidgetItem(*ui->tableWidget->item(i,0)));
            ui->tableUpgraded->setItem(in,1,new QTableWidgetItem(*ui->tableWidget->item(i,2)));
            ui->tableUpgraded->item(in,0)->setText(ui->tableWidget->item(i,7)->text());

            ui->tableUpgraded->setItem(in,2,new QTableWidgetItem(*ui->tableWidget->item(i,4)));
            if(!local){
                ui->tableUpgraded->item(in,1)->setText(ui->tableUpgraded->item(in,1)->toolTip());
            }


            Package p;
            int dsize = local?0:ui->tableWidget->item(i,7)->text().toFloat();
            if(!local)p.setName(ui->tableUpgraded->item(in,1)->toolTip().trimmed().toAscii().data());
            else p.setName(ui->tableWidget->item(i,4)->text().toAscii().data());
            QStringList deps;

            if((pkgs = as->checkDeps(&p,true,false,local))){
                for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
                    QString name;
                    if(!local)name= QString((*it)->getRepository().c_str()).trimmed()+QString("/")+QString((*it)->getName().c_str()).trimmed();
                    else name= QString((*it)->getName().c_str()).trimmed();
                    if((local && name!=ui->tableWidget->item(i,2)->text().trimmed()) || (!local && name!=ui->tableWidget->item(i,2)->toolTip().trimmed())){
                        deps << QString((*it)->getRepository().c_str()).trimmed()+QString("/")+QString((*it)->getName().c_str()).trimmed();
                        dsize+=QString((*it)->getRemoteVersion().c_str()).toFloat()/1024;
                    }
                    delete(*it);
                }
                delete pkgs;
            }


            ui->tableUpgraded->setItem(in,3,new QTableWidgetItem(deps.join(" ")));

            //ui->tableUpgraded->setItem(in++,4,new QTableWidgetItem(*ui->tableWidget->item(i,4)));
            ui->tableUpgraded->setItem(in++,4,new QTableWidgetItem(QString::number(dsize/(float)1024)));
        }else if(ui->tableWidget->item(i,0)->text()=="Upgrade" || (ui->tableWidget->item(i,0)->text()=="Upgradable" && ignoring)){
            //XXX We have to check if the user wants an upgrade before proceding
            ui->tableUpgraded->insertRow(u);            
            ui->tableUpgraded->setItem(u,0,new QTableWidgetItem(*ui->tableWidget->item(i,0)));
            ui->tableUpgraded->item(u,0)->setText(ui->tableWidget->item(i,0)->text());
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

            //ui->tableUpgraded->setItem(u++,4,new QTableWidgetItem(*ui->tableWidget->item(i,4)));

            if(ui->tableWidget->item(i,0)->text()=="Upgradable") ui->tableUpgraded->hideRow(u);

            ui->tableUpgraded->setItem(u,3,new QTableWidgetItem(""));
            ui->tableUpgraded->setItem(u,4,new QTableWidgetItem(QString::number(ui->tableWidget->item(i,7)->text().toFloat()/1024)));

            if(ui->tableWidget->item(i,0)->text()=="Upgrade") u++;


//            ui->tableUpgraded->showRow(u++);
        }
    }

    toI=in;toU=u;toR=r;
    statusI=statusU=statusR=0;    

    confirmRemaining=15;

    if(confirmCountdown){
        ui->editConfirm->setText(tr("Confirm")+QString(" (15)"));
        timerConfirm->start(1000);
    }else{
        ui->editConfirm->setText(tr("Confirm"));
    }
}

QString MainWindow::getDeps(QString pname, bool remote){
    QString ret;
    AS::Package p(!remote);
    p.setName(pname.trimmed().toAscii().data());
    if((pkgs = as->checkDeps(&p,remote))){
        for(std::list<Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
            QString name;
            if(p.isInstalled())
                name = QString((*it)->getName().c_str()).trimmed();
            else
                name = QString((*it)->getRepository().c_str()).trimmed()+QString("/")+QString((*it)->getName().c_str()).trimmed();
            if(name!=pname){
                ret.append(name);

                if(remote){
                    int ksize=QString((*it)->getRemoteVersion().c_str()).toInt()/1024;
                    ret.append(QString(" (")+QString::number(ksize)+QString(" KB)"));
                }

                ret.append("\n");
            }
            delete(*it);
        }
        delete pkgs;
    }
    return ret;
}

void MainWindow::changeStatus(int row, int col){
    if(col && row==currentPacket && row)return;    

    if(ui->tableWidget->item(row,6) && ui->tableWidget->item(row,2) && row!=currentPacket){
        if(ui->urlpre->isChecked() && ui->webView && ui->webView->isEnabled()){
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
            ui->infoText->append(QString("<b>")+tr("Repository")+QString(": </b>")+ui->tableWidget->item(row,1)->text());

        if(!(ui->tableWidget->item(row,0)->text()=="Installed" || ui->tableWidget->item(row,0)->text()=="Remove"))
            ui->infoText->append(QString("<b>"+tr("Size")+QString(": </b>"))+ui->tableWidget->item(row,7)->text()+" KB");

        ui->infoText->append(QString("<b>")+tr("URL")+QString(": </b>")+ui->tableWidget->item(row,6)->text());
        ui->infoText->append(QString("<a href=\"")+ui->tableWidget->item(row,6)->text()+QString("\" >")+QString(tr("(Watch the full site)"))+QString("</a>"));

        if(!(ui->tableWidget->item(row,0)->text()=="Installed" || ui->tableWidget->item(row,0)->text()=="Remove")){
            ui->infoText->append(QString("<b>")+tr("Requires")+QString(":</b>"));
            ui->infoText->append(getDeps(p.getName().c_str(),true));
        }else{
            ui->infoText->append(QString("<b>")+tr("Required by")+QString(":<b>"));
            ui->infoText->append(getDeps(p.getName().c_str(),false));
        }

        QStringList fileStringList;
        if(ui->tableWidget->item(row,0)->text()=="Installed" || ui->tableWidget->item(row,0)->text()=="Remove" || ui->tableWidget->item(row,0)->text()=="Upgrade" || ui->tableWidget->item(row,0)->text()=="Upgradable"){

            //XXX TODO: remove repo/ from pname
            QString pkgName(p.getName().c_str());
            pkgName = pkgName.mid(pkgName.indexOf('/')+1);

            ui->extraInfoGroupBox->setTabIcon(0,QIcon::fromTheme(pkgName));

            ui->extraInfoGroupBox->setTabEnabled(2,true);

            p.setName(pkgName.toAscii().data());

            std::list<std::string>* fileList = ((AS::QTNIXEngine*)as)->getFileList(&p);
            int flsize=fileList->size();
            for(int i=0;i<flsize;++i){
                std::string str = fileList->front();
                fileList->pop_front();
                fileStringList.append(str.c_str());
            }

            ui->fileTreeView->reset();
            if(fileTreeModel) delete fileTreeModel;
            fileTreeModel = 0;
            fileTreeModel = new FileTreeModel(fileStringList, this, &iconProvider);
            ui->fileTreeView->setModel(fileTreeModel);

            ui->fileTreeView->expandAll();

            delete fileList;
        }else{
            ui->extraInfoGroupBox->setTabEnabled(2,false);
            ui->extraInfoGroupBox->setTabIcon(0,QIcon());
        }
    }

    currentPacket = row;

    if(col==0){ //XXXGeneralize
        QMenu menu(this);
        QAction install(QIcon(":pkgstatus/install.png"),tr("Install"),this);
        QAction remove(QIcon(":pkgstatus/remove.png"),tr("Remove"),this);
        QAction upgrade(style()->standardIcon(QStyle::SP_ArrowUp),tr("Upgrade"),this);
        QAction cancel(style()->standardIcon(QStyle::SP_DialogDiscardButton),tr("Cancel"),this);
        QList<QAction*>actions;



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
            install.setVisible(false);
        }else if(text=="Installed"){
            install.setDisabled(true);
            upgrade.setDisabled(true);
        }

        if(text=="Remote"){
            this->install();
        }else if(text=="Installed"){
            this->remove();
        }else if(text=="Remove"){
            this->notRemove();
        }else if(text=="Install"){
            this->notInstall();
        }else if(text=="Upgrade"){
            this->notUpgrade();
        }else{
            menu.exec(QCursor::pos());
        }

        disconnect(&install, SIGNAL(triggered()),this,SLOT(install()));
        disconnect(&remove, SIGNAL(triggered()),this,SLOT(remove()));
        disconnect(&upgrade, SIGNAL(triggered()),this,SLOT(upgrade()));
        disconnect(&cancel, SIGNAL(triggered()),0,0);
    }
}

void MainWindow::showMenu(const QModelIndex & newSelection){
    if(newSelection.column())return;
    QMenu menu(this);
    QAction install(QIcon(":pkgstatus/install.png"),tr("Install"),this);
    QAction remove(QIcon(":pkgstatus/remove.png"),tr("Remove"),this);
    QAction cancel(style()->standardIcon(QStyle::SP_DialogDiscardButton),tr("Cancel"),this);
    QList<QAction*>actions;

    currentPacket = newSelection.row();

    connect(&install, SIGNAL(triggered()),SLOT(installCom()));
    connect(&remove, SIGNAL(triggered()),SLOT(removeCom()));

    QAbstractItemModel *model = ui->tableCommunity->model();
    QString text(model->data(model->index(currentPacket,0)).toString());

    if(text=="Install" || text=="Upgrade" || text=="Remove") actions.append(&cancel);
    else{
        actions.append(&install);
        actions.append(&remove);
    }

    menu.addActions(actions);

    if(text=="Remote"){
        remove.setDisabled(true);
    }else if(text=="Installed"){
        install.setDisabled(true);
    }else if(text=="Remove"){
        connect(&cancel,SIGNAL(triggered(bool)),SLOT(notRemove(bool)));
    }else if(text=="Install"){
        connect(&cancel,SIGNAL(triggered(bool)),SLOT(notInstall(bool)));
    }

    menu.exec(QCursor::pos());

    disconnect(&install, SIGNAL(triggered()),this,SLOT(installCom()));
    disconnect(&remove, SIGNAL(triggered()),this,SLOT(removeCom()));
    disconnect(&cancel, SIGNAL(triggered(bool)),0,0);
}

void MainWindow::updateDB(){
    ui->centralWidget->setDisabled(true);
    ui->mainToolBar->setDisabled(true);    

    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);

    loadingDialog->show();
    loadingBar->setValue(25);
    loadingStatus->setText(tr("UPDATING DB: "));

    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);

    //as->addListener(sbu);
    as->update();
    unlink("/tmp/ashelper.out");
    //as->removeListener(sbu);

    //ui->showUpgradable->setChecked(true);

    loadingBar->setValue(60);
    loadingStatus->setText(tr("DB UPDATED!"));

    QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);

    modified=0;
    applyEnabler();

    merging = false;

    //addRows(true);
    qApp->quit();
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

void MainWindow::showGames(bool community){
    if(community){
        ui->lineEditCommunity->setText("game|fps|fly");
        return;
    }

    category.setPattern("rpg|fps|game|games|race|racing|funny|shooter");
    category_exclude.setPattern("configuration[s]*|kernel|driver[s]*|firmware[s]*|libraries|library|protocol[s]*|dns|debug|traceroute|povray|screenshooter");
    ui->tabWidget->setCurrentIndex(1);
    ui->tabWidget->setTabIcon(1, QIcon(":/pkggroups/games.png"));
    asyncFilter("@@");    
    ui->tabWidget->setTabText(1, tr("Games") );
    ui->showGames_2->setChecked(true);
    ui->showGames_3->setChecked(true);
}

void MainWindow::showSystem(){
    category.setPattern("system|configuration[s]*|kernel|driver[s]*|firmware[s]*|libraries|library|protocol[s]*");
    category_exclude.setPattern("game[s]*|racing");

    ui->tabWidget->setCurrentIndex(1);    
    ui->tabWidget->setTabIcon(1, QIcon(":/pkggroups/system.png"));

    asyncFilter("@@");

    ui->tabWidget->setTabText(1, tr("System") );
    ui->showSystem_2->setChecked(true);
    ui->showSystem_3->setChecked(true);
}

void MainWindow::showMultimedia(){
    category.setPattern("audio|video|player|codec[s]*");
    category_exclude.setPattern("configuration[s]*|kernel|driver[s]*|firmware[s]*|libraries|library|protocol[s]*|rpg|fps|game|games|racing");

    ui->tabWidget->setCurrentIndex(1);
    ui->tabWidget->setTabIcon(1, QIcon(":/pkggroups/multimedia.png"));

    asyncFilter("@@");

    ui->tabWidget->setTabText(1, tr("Multimedia") );
    ui->showMultimedia_2->setChecked(true);
    ui->showMultimedia_3->setChecked(true);
}

void MainWindow::showOffice(){
    category.setPattern(".*office|spreadsheet|word processor");
    category_exclude.setPattern("configuration[s]*|kernel|driver[s]*|firmware[s]*|libraries|library|protocol[s]*");

    ui->tabWidget->setCurrentIndex(1);
    ui->tabWidget->setTabIcon(1, QIcon(":/pkggroups/office.png"));

    asyncFilter("@@");

    ui->tabWidget->setTabText(1, tr("Office") );
    ui->showOffice_2->setChecked(true);
    ui->showOffice_3->setChecked(true);
}

void MainWindow::showInternet(){
    category.setPattern("email|mail|browser|mozilla|google|opera");
    category_exclude.setPattern("gimp|image|configuration[s]*|kernel|driver[s]*|firmware[s]*|libraries|library|protocol[s]*");

    ui->tabWidget->setCurrentIndex(1);
    ui->tabWidget->setTabIcon(1, QIcon(":/pkggroups/internet.png"));

    asyncFilter("@@");

    ui->tabWidget->setTabText(1, tr("Internet") );
    ui->showInternet_2->setChecked(true);
    ui->showInternet_3->setChecked(true);
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

    ui->tabWidget->setTabText(1, tr("All") );
    ui->showAllCat_2->setChecked(true);
    ui->showAllCat_3->setChecked(true);
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

void MainWindow::repoFilter(){
    /*int rows = ui->tableWidget->rowCount();

    for(int i=0;i<rows;++i){
        if(isRepoFiltered(ui->tableWidget->item(i,1)->text())){
            ui->tableWidget->hideRow(i);
        }
    }*/

    asyncFilter("@@");
}

bool MainWindow::isRepoFiltered(const QString &repoName){
    return (ui->repoFilter->currentText()!=tr("All") && (repoName=="NO INFO"?((AS::QTNIXEngine*)as)->getCommunityName().c_str():repoName)!=(ui->repoFilter->currentText()));
}

void MainWindow::clearPackagesList(){
    int count = ui->tableWidget->rowCount();
    for(int i=0;i<count;++i)ui->tableWidget->removeRow(0);    
}

void MainWindow::addRows(bool checked){
    if(!checked) return;

    if(pp>=1 && pp<=3){
        baseIndex=0;
        baseSizes.reserve(toI+toU);
        completed.reserve(toI+toU);
        logger=new ASLogger();
        as->addListener(logger);
        for(int k=0;k<toI+toU;++k){
            baseSizes[k]=-1;
            completed[k]=false;
        }
        editConfirm();
        return;
    }else if(pp>=6 && pp<=8){
        comCommon(pp); //upgrade com
        return;
    }else if(pp==5){
        this->hide();
        updateDB();
        return;
    }else if(pp==10){
        ((AS::QTNIXEngine*)as)->cleanCache();
        qApp->quit();
        return;
    }

    QString param("");
    if(!argsParsed && qApp->argc()>1){
        param=QString((qApp->argv())[1]);
        argsParsed=true;
        openLocal(param);
        return;
    }

    tpack=ipack=upack=epack=0;
    local=false;

    int rows=0;upgradables=0;

    ui->centralWidget->setDisabled(true);
    ui->mainToolBar->setDisabled(true);

    //ui->stacked->setCurrentIndex(2);
    if(this->isVisible())loadingDialog->show();
    loadingBar->setValue(0);    

    timer2->stop();

    loadingStatus->setText(tr("Loading packages..."));

    QCoreApplication::processEvents(QEventLoop::AllEvents, 500);

    QTableWidgetItem *newItem;


    //ui->tableWidget->clearContents();
    //int count = ui->tableWidget->rowCount();
    //for(int i=0;i<count;++i)ui->tableWidget->removeRow(0);
    //ui->tableWidget->setRowCount(0);
    clearPackagesList();

    ui->tableWidget->setSortingEnabled(false);

    std::list<AS::Package*> *ipkgs=new std::list<AS::Package*>();

    bool tool_up=false;

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
            ui->tableWidget->setItem(i,3, new QTableWidgetItem(""));
            ui->tableWidget->setItem(i,4, newItem);
            newItem = new QTableWidgetItem(pkg->getDescription().c_str());
            ui->tableWidget->setItem(i,5, newItem);
            newItem = new QTableWidgetItem(pkg->getURL().c_str());
            ui->tableWidget->setItem(i,6, newItem);
            newItem = new QTableWidgetItem(QString::number(pkg->getSize()/1024));
            ui->tableWidget->setItem(i,7, newItem);

            i++;

            delete pkg;

            if(!(i%100)){
                loadingBar->setValue(30+i*20/pkgs->size());
                QCoreApplication::processEvents(QEventLoop::AllEvents, 33);
            }            
        }

        //pkgs->clear();
        delete pkgs;
        pkgs=0;

        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        sbu.setPreMessage(tr("PARSING INSTALLED: "));
        sbu.setStepping(60);
        t2.wait();
        pkgs=t2.getList();
        ipack=pkgs->size();
        loadingStatus->setText(tr("Searching correspondeces..."));

        int k=0;
        rows = ui->tableWidget->rowCount();
        QList<int> toRemove;
        for(std::list<Package*>::iterator it=pkgs->begin(); it!=pkgs->end(); it++){
            Package *pkg = *it;

            newItem = new QTableWidgetItem("");
            ui->tableWidget->setItem(i,1, newItem);

            int found=-1;
            //QTableWidgetItem* versionMatch;
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
                ipkgs->push_back(new AS::Package(*pkg));
                ui->tableWidget->insertRow(i);
                newItem = new QTableWidgetItem(QIcon(":pkgstatus/checked.png"),"Installed");
                newItem->setToolTip(tr("Installed (external)"));
                ui->tableWidget->setItem(i,0, newItem);                
                newItem = new QTableWidgetItem(pkg->getRepository().c_str());
                ui->tableWidget->setItem(i,1, newItem);
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
                newItem = new QTableWidgetItem(QString::number(pkg->getSize()/1024));
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

        //ui->statusBar->showMessage("CHECKING UPGRADABLES", 3000);
        loadingStatus->setText(tr("Checking upgradables"));

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

                        if(ui->tableWidget->item(i,2)->text()==QString(((AS::QTNIXEngine*)as)->getTool().c_str())){
                            tool_up=true;
                            currentPacket=i;
                        }
                    }

                    if(ups)upsit=ups->begin();
                }
                loadingBar->setValue(95+i*5/rows);
            }


            delete ups;
        }
        //pkgs->clear();
        delete pkgs;
        pkgs=0;
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

                    if(((AS::QTNIXEngine*)as)->getTool().compare(pkg->getName()) == 0){
                        tool_up = true;
                        currentPacket = i;
                    }
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

            QString repo(pkg->getRepository().c_str());
            newItem = new QTableWidgetItem(repo);
            ui->tableWidget->setItem(i,1, newItem);            

            newItem = new QTableWidgetItem(pkg->getName().c_str());
            newItem->setToolTip((QString(pkg->getRepository().c_str())+QString("/")+QString(pkg->getName().c_str())));
            ui->tableWidget->setItem(i,2, newItem);

            RepoStats *repoInstance = &repoStats[repo];
            repoInstance->total++;

            if(pkg->isInstalled()){
                newItem = new QTableWidgetItem(pkg->getLocalVersion().c_str());
                ui->tableWidget->setItem(i,3, newItem);

                repoInstance->installed++;
            }else{
                ui->tableWidget->setItem(i,3, new QTableWidgetItem(""));
            }

            if(pkg->getRemoteVersion().compare("NO INFO")){
                newItem = new QTableWidgetItem(pkg->getRemoteVersion().c_str());
            }else{
                ipkgs->push_back(new AS::Package(*pkg));
                newItem = new QTableWidgetItem((QString(" (")+tr("External")+QString(")")));
                epack++;
            }
            ui->tableWidget->setItem(i,4, newItem);

            newItem = new QTableWidgetItem(pkg->getDescription().c_str());
            ui->tableWidget->setItem(i,5, newItem);

            newItem = new QTableWidgetItem(pkg->getURL().c_str());
            ui->tableWidget->setItem(i,6, newItem);

            newItem = new QTableWidgetItem(QString::number(pkg->getSize()/1024));
            ui->tableWidget->setItem(i,7, newItem);

            i++;

            delete pkg;

            if(!(i%20)){
                loadingBar->setValue(i*100/pkgs->size());
                QCoreApplication::processEvents(QEventLoop::AllEvents, 33);
            }

            it++;
        }

        //pkgs->clear();
        delete pkgs;
        pkgs=0;
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
    if(!tool_up){
        ui->tableWidget->setSortingEnabled(true);
        asyncFilter();
    }
    //timer2->start();

    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    //ui->stacked->setCurrentIndex(0);
    loadingDialog->hide();
    loadingBar->setValue(0);

    ui->centralWidget->setEnabled(true);
    ui->mainToolBar->setEnabled(true);

    merging = true;

    //Repos filter
    QStringList repos(repoStats.keys());
    repos.sort();
    ui->repoFilter->addItems(repos);
    ui->repoFilter->setItemText(ui->repoFilter->findText("NO INFO"),((AS::QTNIXEngine*)as)->getCommunityName().c_str());

    //Statistics
    QStringList statText;        
    for(int j=0;j<repos.size();++j){
        if(repos.at(j)=="NO INFO") continue;
        statText.append(QString("<b>")+repos.at(j)+QString("</b>"));
        statText.append(tr("Packages")+QString(":\t")+QString::number(repoStats[repos.at(j)].total));
        statText.append(tr("Installed")+QString(":\t")+QString::number(repoStats[repos.at(j)].installed)+QString("<br>"));
    }
    statText.append(QString("<b>")+QString(((AS::QTNIXEngine*)as)->getCommunityName().c_str())+QString("</b>"));
    statText.append(tr("Installed")+QString(":\t")+QString::number(repoStats["NO INFO"].installed));
    statText.append(QString("<b>"));
    statText.append(tr("Total")+QString("</b>"));
    statText.append(tr("Packages")+QString(":\t")+QString::number(tpack));
    statText.append(tr("Installed")+QString(":\t")+QString::number(ipack));
    ui->statText->setHtml(statText.join("<br>"));

    if(ui->tabWidget->tabText(1)==tr("All"))ui->tabWidget->setTabText(1, tr("All"));

    emit installedPackagesUpdated(ipkgs);

    if(!((AS::QTNIXEngine*)as)->isCommunityEnabled() && ((AS::QTNIXEngine*)as)->getCommunityToolName().find("*")==std::string::npos)
        ui->statusBar->showMessage(tr("To enable external packages support you have to install")+QString(" ")+
                                   QString(((AS::QTNIXEngine*)as)->getCommunityToolName().c_str()),5000);
    else
        ui->statusBar->showMessage("");

    if(tool_up) upgrade_tool();

    loadingStatus->setText("");
}


void MainWindow::upgrade_tool(){
#ifdef unix
    QMessageBox con;
    con.setText(tr("The main backend")+QString(" (")+QString(((AS::QTNIXEngine*)as)->getTool().c_str())+QString(") ")+
                tr("is not up to date. You have to upgrade it in order to continue with other operations.")+QString("\n")+
                tr("Do you want to proceed now?"));
    con.setIcon(QMessageBox::Question);
    con.setStandardButtons(QMessageBox::Yes|QMessageBox::No);

    inModal=true;
    if(con.exec()==QMessageBox::Yes){
        bool preExpert=isExpert;
        isExpert=true;
        upgrade();
        confirm();
        getPrivileges();
        isExpert=preExpert;        
    }else{
        QCoreApplication::quit();
    }
    inModal=false;
#endif
}

void MainWindow::bugReport(){
    extBrowserLink(QUrl("http://sourceforge.net/tracker/?func=browse&group_id=376825&atid=1568693"));
}

void MainWindow::featureRequest(){
    extBrowserLink(QUrl("http://sourceforge.net/tracker/?func=browse&group_id=376825&atid=1568696"));
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

    if(pp && pp!=9 && pp!=11){
        return;
    }

    QFile::remove("/tmp/asshown");

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
