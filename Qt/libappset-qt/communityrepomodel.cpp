#include "communityrepomodel.h"

CommunityRepoModel::CommunityRepoModel(QObject *parent, AS::QTNIXEngine *engine) :
    QAbstractTableModel(parent), engine(engine)
{
    pkgs=0;
}

int CommunityRepoModel::rowCount(const QModelIndex &parent) const{
    if(pkgs) return pkgs->size();
    return 0;
}

int CommunityRepoModel::columnCount(const QModelIndex &parent) const{
    return 3;
}

QVariant CommunityRepoModel::data(const QModelIndex &index, int role) const{
    if(!pkgs || !pkgs->size())return QVariant();

    switch(role){
        case Qt::DisplayRole:
            switch(index.column()){
                case 0:
                    return QString(pkgs->at(index.row())->getName().c_str());
                    break;
                case 1:
                    return QString(pkgs->at(index.row())->getRemoteVersion().c_str());
                    break;
                case 2:
                    return QString(pkgs->at(index.row())->getDescription().c_str());
                    break;
            }

            break;
    }

    return QVariant();
}

void CommunityRepoModel::selectionChangedSlot(const QModelIndex &newSelection, const QModelIndex &oldSelection){
    QString pname = data(createIndex(newSelection.row(),0),Qt::DisplayRole).toString();

    std::list<AS::Package*>* plist=engine->com_info(pname.toAscii().data());

    if(plist && plist->size()) emit pkgInfoRetrieved(plist->front());

    if(plist) delete plist;
}

void CommunityRepoModel::setPattern(QString pattern){
    this->pattern=pattern;

    std::list<AS::Package*>* plist=engine->com_search(pattern.toAscii().data());

    if(pkgs){
        beginRemoveRows(QModelIndex(),0,rowCount()-1);
        for(std::vector<AS::Package*>::iterator it=pkgs->begin();it!=pkgs->end();it++){
            delete (*it);
        }
        pkgs->clear();
        endRemoveRows();
    }

    if(plist&&plist->size()){
        beginInsertRows(QModelIndex(),0,plist->size()-1);
        pkgs=new std::vector<AS::Package*>();
        for(std::list<AS::Package*>::iterator it=plist->begin();it!=plist->end();it++){
            pkgs->push_back(*it);
        }
        endInsertRows();

        if(plist)delete plist;
    }

    //emit dataChanged(createIndex(0,0),createIndex(rowCount()-1,2));
}
