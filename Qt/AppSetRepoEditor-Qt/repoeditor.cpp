/*
Copyright 2011 Simone Tobia

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
#include "repoeditor.h"
#include "ui_repoeditor.h"

#include "repoconf.h"
#include "checkboxdelegate.h"
#include "optionsdelegate.h"

#include <QFileDialog>
#include <QMessageBox>

RepoEditor::RepoEditor(QString confFile, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RepoEditor)
{
    ui->setupUi(this);

    repoConf = new RepoConf(confFile);
    addRepoDialog = new AddRepo(this);

    ui->tableView->setModel(repoConf);
    ui->tableView->setItemDelegateForColumn(0,new CheckBoxDelegate(this));
    ui->tableView->setItemDelegateForColumn(2,new OptionsDelegate(this));

    ui->loadBackup->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    ui->moveDown->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    ui->moveUp->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    ui->remove->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    ui->add->setIcon(QIcon(":actions/add.png"));

    ui->tableView->setColumnWidth(1,133);

    ui->backupFile->setText(repoConf->getConfPath()+".bak");

    connect(ui->moveUp,SIGNAL(clicked()),SLOT(moveUp()));
    connect(ui->moveDown,SIGNAL(clicked()),SLOT(moveDown()));

    QItemSelectionModel *selModel = ui->tableView->selectionModel();
    connect(selModel,SIGNAL(selectionChanged(QItemSelection,QItemSelection))
            ,SLOT(updateMovers(QItemSelection,QItemSelection)));

    connect(ui->buttonBox->button(QDialogButtonBox::Reset),SIGNAL(clicked()),repoConf,SLOT(reload()));

    connect(ui->buttonBox->button(QDialogButtonBox::Save),SIGNAL(clicked()),SLOT(apply()));
    connect(ui->buttonBox->button(QDialogButtonBox::Discard),SIGNAL(clicked()),SLOT(discard()));

    connect(ui->remove,SIGNAL(clicked()),SLOT(removeEntry()));
    connect(ui->add,SIGNAL(clicked()),SLOT(addEntry()));

    connect(ui->loadBackup,SIGNAL(clicked()),SLOT(loadBackup()));

    ui->tableView->selectRow(0);
}

RepoEditor::~RepoEditor()
{
    delete ui;
}

void RepoEditor::loadBackup(){
    RepoConf conf;
    QString file=QFileDialog::getOpenFileName(this);

    if(file.isEmpty()) return;

    conf.loadConf(file);
    if(!conf.count()){
        QMessageBox mb(QMessageBox::Critical,tr("Can't load backup file"),tr("Selected file is not valid"),QMessageBox::Ok,this);
        mb.exec();
    }else{
        repoConf->loadConf(file);
        ui->backupFile->setText(repoConf->getConfPath()+".bak");
    }
}

void RepoEditor::addEntry(){
    if(addRepoDialog->exec()==QDialog::Accepted){
        if(!repoConf->exists(addRepoDialog->entry.getName()))
            repoConf->addEntry(addRepoDialog->entry);
    }
}

void RepoEditor::apply(){
    if(repoConf->saveChanges(ui->checkBox->isChecked()?ui->backupFile->text():"")){
        QMessageBox::information(this,tr("Success"),tr("Repositories configuration successfully saved."),QMessageBox::Ok);
        done(QDialog::Accepted);
    }else{
        QMessageBox::critical(this,tr("Error"),tr("Repositories configuration not saved."),QMessageBox::Ok);
    }
}

void RepoEditor::discard(){
    done(QDialog::Rejected);
}

void RepoEditor::updateMovers(const QItemSelection &cur,const QItemSelection &pre){
    QModelIndexList list=cur.indexes();
    if(!list.count())return;
    int row=list.at(0).row();

    if(row>0)ui->moveUp->setEnabled(true);
    else ui->moveUp->setEnabled(false);

    if(row<ui->tableView->model()->rowCount()-1)ui->moveDown->setEnabled(true);
    else ui->moveDown->setEnabled(false);
}

void RepoEditor::moveUp(){
    QModelIndexList list=ui->tableView->selectionModel()->selectedRows();
    if(!list.count())return;
    int row=list.at(0).row();
    repoConf->moveUp(row);
    ui->tableView->selectRow(row-1);
}

void RepoEditor::moveDown(){
    QModelIndexList list=ui->tableView->selectionModel()->selectedRows();
    if(!list.count())return;
    int row=list.at(0).row();
    repoConf->moveDown(row);
    ui->tableView->selectRow(row+1);
}

void RepoEditor::removeEntry(){
    QModelIndexList list=ui->tableView->selectionModel()->selectedRows();
    if(!list.count())return;
    int row=list.at(0).row();
    repoConf->removeRow(row);
}
