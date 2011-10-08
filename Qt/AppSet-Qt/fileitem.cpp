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

#include <QString>
#include <QList>

#include "fileitem.h"

FileItem::FileItem(const QString &data, FileItem *parent, bool isFile){
    itemData = data;
    parentItem = parent;
    isFileItem = isFile;
}

FileItem::~FileItem(){
    qDeleteAll(childItems);
}

void FileItem::appendChild(FileItem *child){
    childItems.append(child);
}

FileItem* FileItem::child(int row){
    return childItems.value(row);
}

int FileItem::childCount() const{
    return childItems.count();
}

QString FileItem::data() const{
    return itemData;
}

int FileItem::row() const{
    if(parentItem){
        return parentItem->childItems.indexOf(const_cast<FileItem*>(this));
    }

    return 0;
}

FileItem* FileItem::parent(){
    return parentItem;
}


bool FileItem::isFile() const{
    return isFileItem;
}
