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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QThread>
#include <QTableWidget>
#include <QResizeEvent>
#include <QMessageBox>
#include <QThreadPool>
#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QMovie>
#include <asengine.h>
#include <list>
#include <vector>

#include <QEventLoop>

#include <QCompleter>

#ifdef unix
#include "communityrepomodel.h"
#endif

namespace Ui {
    class MainWindow;
}

#include <QStatusBar>
#include <QCoreApplication>

class AsThread:public QThread{
    int op;
    std::list<AS::Package*> *l;
    AS::EngineListener *el;
    QTableWidget *table;
    AS::Engine *as;    
public:
    int status;
    bool local;
    QString pattern;
    AsThread(AS::Engine *as){
        this->l=0;
        this->op=0;
        status=0;
        this->as=as;
        local=false;
    }

    int getStatus(){return status;}
    void setOp(int op){this->op=op;}
    void setList(std::list<AS::Package*> *l){this->l=l;}
    void setEL(AS::EngineListener *el){this->el=el;}
    AS::EngineListener* getEl(){return el;}
    std::list<AS::Package*>* getList(){return l;}
    void setTable(QTableWidget* table){this->table=table;}
    QTableWidget* getTable(){return table;}
    int getOp(){return op;}

    void run(){
        switch(op){
        case 1:
            status+=as->install(l,local);
            break;
        case 2:
            status+=as->upgrade(l);
            break;
        case 3:
            status+=as->remove(l);
            break;
        case 4:
            l=as->queryLocal(as_QUERY_ALL_INFO | as_EXPERT_QUERY);
            break;
        case 5:
            l=as->queryRemote(as_QUERY_ALL_INFO | as_EXPERT_QUERY);
            break;
        case 6:
            status+=((AS::QTNIXEngine*)as)->com_install(pattern.toAscii().data());
            break;
        case 7:
            status+=((AS::QTNIXEngine*)as)->com_remove(pattern.toAscii().data());
            break;
        case 8:
            status+=((AS::QTNIXEngine*)as)->com_upgrade(pattern.toAscii().data());
            break;
        }
    }
};

#include <fstream>
class ASLogger:public AS::EngineListener{
    std::ofstream logFile;
public:
    ASLogger(){
#ifdef unix
        logFile.open("/var/log/appset.log");
        logFile.write("",1);
#endif
    }
    ~ASLogger(){
#ifdef unix
        if(logFile.is_open()) logFile.close();
#endif
    }

    void step(const char *content){
        logFile << std::string(content) << std::endl;
        logFile.flush();
    }
};

class StatusBarUpdater:public AS::EngineListener{
    QStatusBar *bar;
    QString pre;
    int stepping;
    int i;
    QProgressBar *lb;
public:
    StatusBarUpdater(QStatusBar *bar){
        this->bar=bar;i=0;stepping=3333;pre=QString("PARSING DATABASE: ");
    }
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


//RSS feeder
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QBuffer>
#include <QXmlStreamReader>
#include <QUrl>

#include <QMultiMap>

#include "options.h"

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void filterPressed();
    void applyEnabler();


public slots:
    void addRows(bool checked=true);
    void changeStatus(int row, int col);
    void updateDB();

    void showAll(bool checked=true);
    void showUpgradable(bool checked=true);
    void showInstalled(bool checked=true);
    void showNotInstalled(bool checked=true);

    void showGames(bool community=false);
    void showAllCat();
    void showSystem();
    void showMultimedia();
    void showOffice();
    void showInternet();

    void showOptions();

    void expertMode(bool);
    //void searchFilter(QString filter);
    void asyncFilter(QString filter="@");
    void searchTermChanged(int);
    void markUpgrades();

    void install(bool community=false);

    void installCom();
    void removeCom();
    void upgradeCom();

    void remove(bool community=false);
    void upgrade(bool community=false);
    void notInstall(bool community=false);
    void notRemove(bool community=false);
    void notUpgrade(bool community=false);
    void confirm();

    void cleanCache();

    void editConfirm();
    void editCancel();

    void timerFired(QString);
    void timeFilter();

    void comTimerFired(QString);
    void comTimeFilter();

    void refresh();
    void refreshCom();

    void opFinished();

    void aboutQt();
    void about();

    void confirmTimeout();

    void comOpFinished();
    void comContinued();

    //RSS
    void finished(QNetworkReply *reply);
    void readyRead();
    void metaDataChanged();
    void itemActivated(QTreeWidgetItem * item);
    void error(QNetworkReply::NetworkError);

    void extBrowserLink(const QUrl & link);

    void openLocal(QString fileName="");

    void comInfoRetrieved(AS::Package *pkg);
    void comTableUpdated();

    void showMenu(const QModelIndex & newSelection);

    void clearComLine();
private:
    Ui::MainWindow *ui;

    AS::Engine *as;

    QTimer *timer;
    QTimer *timer2;
    QTimer *timerUpdate;

    QTimer *timerComSearch;
    QTimer *timerUpdateCom;

    QTimer *timerConfirm;
    int confirmRemaining;

    std::list<AS::Package*> *pkgs;

    unsigned flags;

    int currentPacket;

    int modified;

    QAction *applyAction;
    QAction *markAction;
    QAction *cleanAction;
    QAction *openLocalAction;

    int toI, toU, toR;
    int statusI, statusU, statusR;

    QRegExp category;
    QRegExp category_exclude;
    QRegExp expert;

    bool isExpert;

    AsThread *asThread;
#ifdef unix
    AsThread *asComThread;
#endif
    QDialog *loadingDialog;
    QProgressBar *loadingBar;

    std::vector<int> baseSizes;
    std::vector<bool> completed;
    int baseIndex;

    QMovie *loadingMovie;

    int visibleRowCount();

    bool merging;

    ASLogger *logger;

    StatusBarUpdater *sbu;

    QNetworkReply *currentReply;
    QNetworkAccessManager manager;
    QXmlStreamReader xml;
    QString currentTag;
    QString linkString;
    QString titleString;
    QString descString;

    void parseXml();
    void get(const QUrl &url);

    int sbdelay;
    QString extbrowser;
    bool showBackOut;
    bool confirmCountdown;

    QMultiMap<QString, QString> instaDeps;
    QMultiMap<QString, QString> remDeps;

    int tpack;
    int ipack;
    int upack;
    int epack;

    int local;
    int argsParsed;

    //Outcome evaluation
    QStringList iOutcome,rOutcome,uOutcome;

    bool outcomeEvaluator();

    QCompleter *pcomp;

    void comCommon(int op);


signals:
    void installedPackagesUpdated(std::list<AS::Package*> *);
    void comPatternUpdated(QString);
};

#endif // MAINWINDOW_H
