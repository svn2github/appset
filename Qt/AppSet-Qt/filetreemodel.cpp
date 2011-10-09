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

#include <QStringList>
#include <QString>

#include <QStyle>
#include <QApplication>

#include "filetreemodel.h"

FileTreeModel::FileTreeModel(const QStringList &data, QObject *parent, QFileIconProvider *iconProvider)
    : QAbstractItemModel(parent) {
    //Create the root element
    rootItem = new FileItem("/");

    this->iconProvider = iconProvider;

    //Populate the data model
    setupModelData(data, rootItem);
}

FileTreeModel::~FileTreeModel(){
    delete rootItem;
}


QModelIndex FileTreeModel::index(int row, int column, const QModelIndex &parent) const{
    if(!hasIndex(row,column,parent)){
        return QModelIndex();
    }

    FileItem *parentItem;

    if(!parent.isValid()){
        parentItem = rootItem;
    }else{
        parentItem = (FileItem*) parent.internalPointer();
    }

    FileItem *childItem = parentItem->child(row);

    if(childItem){
        return createIndex(row, column, childItem);
    }

    return QModelIndex();
}

QModelIndex FileTreeModel::parent(const QModelIndex &index) const{
    if(!index.isValid()){
        return QModelIndex();
    }

    FileItem *childItem = (FileItem*) index.internalPointer();
    FileItem *parentItem = childItem->parent();

    if(parentItem == rootItem){
        return QModelIndex();
    }

    return createIndex(parentItem->row(),0,parentItem);
}

int FileTreeModel::rowCount(const QModelIndex &parent) const{
    FileItem *parentItem;

    if(parent.column()>0) return 0;

    if(!parent.isValid()){
        parentItem = rootItem;
    }else{
        parentItem = (FileItem*) parent.internalPointer();
    }

    return parentItem->childCount();
}

int FileTreeModel::columnCount(const QModelIndex &parent) const{
    return 1;
}

#include <QFileIconProvider>
#include <QFileInfo>
QVariant FileTreeModel::data(const QModelIndex &index, int role) const{
    if(!index.isValid()) return QVariant();

    FileItem *item = (FileItem*) index.internalPointer();

    if(role == Qt::DisplayRole){
        return item->data();
    }else if(role == Qt::DecorationRole){
        if(item->isFile()){
            QStringList filePath;
            FileItem *cur=item;
            while( cur ){
                filePath.push_front(cur->data());
                cur=cur->parent();
            }
            QString file('/');
            file += filePath.join("/");

            if(iconProvider){
                return iconProvider->icon(QFileInfo(file));
            }else{
                return qApp->style()->standardIcon(QStyle::SP_FileIcon);
            }
        }else{
            return qApp->style()->standardIcon(QStyle::SP_DirIcon);
        }
    }

    return QVariant();
}

Qt::ItemFlags FileTreeModel::flags(const QModelIndex &index) const{
    if(!index.isValid()) return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant FileTreeModel::headerData(int section, Qt::Orientation orientation, int role) const{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole){
        return rootItem->data();
    }

    return QVariant();
}

void FileTreeModel::setupModelData(const QStringList &files, FileItem *parent){
    int count = files.count();
    int depth = 1;
    FileItem *item = parent;

    for( int i=0; i<count; ++i ){
        QString newPath = files.at(i);
        bool isLeaf = !newPath.endsWith('/');
        int newDepth = newPath.count('/') + ( isLeaf ? 1 : 0 );
        QString itemData = newPath.mid(newPath.lastIndexOf('/',-2)+1);        

        FileItem *realParent = item;
        for( int j=newDepth; j<=depth; ++j){
            realParent = realParent->parent();
        }

        if( !isLeaf ) itemData.chop(1);

        FileItem *newItem= new FileItem(itemData, realParent, isLeaf);
        realParent->appendChild( newItem );

        item = newItem;
        depth = newDepth;
    }
}
