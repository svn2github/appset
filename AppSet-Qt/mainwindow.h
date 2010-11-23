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
#include <asengine.h>
#include <list>
#include <vector>

namespace Ui {
    class MainWindow;
}

class AsThread:public QThread{
    int op;
    std::list<AS::Package*> *l;
    AS::EngineListener *el;
    QTableWidget *table;
    AS::Engine *as;
public:
    int status;
    AsThread(AS::Engine *as){
        this->l=0;
        this->op=0;
        status=0;
        this->as=as;
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
            status+=as->install(l);
            break;
        case 2:
            status+=as->upgrade(l);
            break;
        case 3:
            status+=as->remove(l);
            break;
        }
    }
};

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

    void showGames();
    void showAllCat();
    void showSystem();
    void showMultimedia();
    void showOffice();
    void showInternet();

    void expertMode(bool);
    //void searchFilter(QString filter);
    void asyncFilter(QString filter="@");
    void searchTermChanged(int);
    void markUpgrades();

    void install();
    void remove();
    void upgrade();
    void notInstall();
    void notRemove();
    void notUpgrade();
    void confirm();

    void editConfirm();
    void editCancel();

    void timerFired(QString);
    void timeFilter();

    void refresh();

    void opFinished();

    void aboutQt();
    void about();

private:
    Ui::MainWindow *ui;

    AS::Engine *as;

    QTimer *timer;
    QTimer *timer2;
    QTimer *timerUpdate;

    std::list<AS::Package*> *pkgs;

    unsigned flags;

    int currentPacket;

    int modified;

    QAction *applyAction;

    int toI, toU, toR;
    int statusI, statusU, statusR;

    QRegExp category;
    QRegExp category_exclude;

    AsThread *asThread;

    QDialog *loadingDialog;
    QProgressBar *loadingBar;

    std::vector<int> baseSizes;
    int baseIndex;
};

#endif // MAINWINDOW_H
