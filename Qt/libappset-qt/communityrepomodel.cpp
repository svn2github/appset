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

#include "communityrepomodel.h"

CommunityRepoModel::CommunityRepoModel(QStringList headers, QObject *parent, AS::QTNIXEngine *engine) :
    QAbstractTableModel(parent), engine(engine), headers(headers)
{
    pkgs=0;
    ipkgs=0;
}

int CommunityRepoModel::rowCount(const QModelIndex &parent) const{
    if(pkgs) return pkgs->size();
    return 0;
}

int CommunityRepoModel::columnCount(const QModelIndex &parent) const{
    return 5;
}

#include <QIcon>
QVariant CommunityRepoModel::data(const QModelIndex &index, int role) const{
    if(!pkgs || !pkgs->size())return QVariant();

    switch(role){
        case Qt::DisplayRole:
            switch(index.column()){
                case 0:
                    return QString(pkgs->at(index.row())->isInstalled()?"Installed":"Remote");
                    break;
                case 1:
                    return QString(pkgs->at(index.row())->getName().c_str());
                    break;
                case 2:
                    return QString(pkgs->at(index.row())->isInstalled()?pkgs->at(index.row())->getLocalVersion().c_str():"");
                    break;
                case 3:
                    return QString(pkgs->at(index.row())->getRemoteVersion().c_str());
                    break;
                case 4:
                    return QString(pkgs->at(index.row())->getDescription().c_str());
                    break;
            }

            break;

        case Qt::DecorationRole:
            if(!index.column()){
                return QIcon(pkgs->at(index.row())->isInstalled()?":/pkgstatus/checked.png":":/pkgstatus/unchecked.png");
            }
            break;
    }

    return QVariant();
}

QVariant CommunityRepoModel::headerData(int section, Qt::Orientation orientation, int role) const{
    switch(role){
        case Qt::DisplayRole:
            if(orientation==Qt::Horizontal){
                return headers.at(section);
            }
            break;
    }

    return QVariant();
}

void CommunityRepoModel::selectionChangedSlot(const QModelIndex &newSelection, const QModelIndex &oldSelection){
    QString pname = data(createIndex(newSelection.row(),1),Qt::DisplayRole).toString();

    std::list<AS::Package*>* plist=engine->com_info(pname.toAscii().data());

    if(plist && plist->size()) emit pkgInfoRetrieved(plist->front());

    if(plist) delete plist;
}

void CommunityRepoModel::setPattern(QString pattern){
    this->pattern=pattern;

    std::list<AS::Package*>* plist=engine->com_search(pattern.toAscii().data());

    if(pkgs){
        beginResetModel();
        pkgs->clear();
        if(pkgs){
            delete pkgs;
            pkgs=0;
        }
        endResetModel();
    }

    if(plist&&plist->size()){        
        pkgs=new std::vector<AS::Package*>();
        beginInsertRows(QModelIndex(),0,plist->size()-1);
        for(std::list<AS::Package*>::iterator it=plist->begin();it!=plist->end();it++){
            pkgs->push_back(*it);
        }
        endInsertRows();

        if(plist)delete plist;
    }

    mergeInstalled();

    emit dataUpdated();
}

void CommunityRepoModel::setInstalledPackages(std::list<AS::Package *> *pkgs){
    if(ipkgs){
        ipkgs->clear();
        delete ipkgs;
        ipkgs=0;
    }
    ipkgs=pkgs;

    //mergeInstalled();
    setPattern(pattern);
}
#include <QStringList>
void CommunityRepoModel::mergeInstalled(){
    if(ipkgs && ipkgs->size()){
        if(pkgs && pkgs->size()){
            for(unsigned int i=0;i<pkgs->size();++i){
                for(std::list<AS::Package*>::iterator it2=ipkgs->begin();it2!=ipkgs->end();it2++){
                    if(QString(pkgs->at(i)->getName().c_str())==QString((*it2)->getName().c_str())){
                        pkgs->at(i)->setInstalled(true);
                        pkgs->at(i)->setLocalVersion((*it2)->getLocalVersion());

                        emit dataChanged(createIndex(i,0),createIndex(i,3));
                    }
                }
            }
        }else{
            /*if(pkgs) delete pkgs;

            pkgs=new std::vector<AS::Package*>();
            beginInsertRows(QModelIndex(),0,ipkgs->size()-1);
            for(std::list<AS::Package*>::iterator it=ipkgs->begin();it!=ipkgs->end();it++){

                pkgs->push_back(*it);

            }
            endInsertRows()*/;
        }
    }

}
