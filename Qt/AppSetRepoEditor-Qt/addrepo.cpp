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
#include "addrepo.h"
#include "ui_addrepo.h"

#include <QMessageBox>

AddRepo::AddRepo(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddRepo)
{
    ui->setupUi(this);

    connect(ui->buttonBox,SIGNAL(accepted()),SLOT(checkAndApply()));
}

AddRepo::~AddRepo()
{
    delete ui;
}


void AddRepo::checkAndApply(){
    if(ui->repo->text().remove(RepoConf::commentString).trimmed().isEmpty()){
        showError(tr("The repository name field can't be blank."));
    }else if(RepoConf::matchRepo(RepoEntry::formatRepoName(ui->repo->text()))){
        QStringList list=ui->option->toPlainText().split("\n");
        bool valid=true;
        for(int i=0;valid && i<list.count();++i){
            list[i]=list.at(i).trimmed();
            if(!RepoConf::matchRepoDetails(list.at(i))) valid=false;
        }

        if(valid){
            entry.setName(ui->repo->text());
            entry.setDetails(ui->option->toPlainText().split("\n"));
            done(QDialog::Accepted);
        }else{
            showError(tr("The repository options field is not valid."));
        }
    }else{
        showError(tr("The repository name field is not valid."));
    }
}

void AddRepo::showError(QString error){
    QMessageBox mb(QMessageBox::Critical,tr("Error"),tr("Can't add repository.")+QString("\n")+error,QMessageBox::Ok,this);
    mb.exec();
}
