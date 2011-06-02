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
#include "appitem.h"

AppItem::AppItem(QObject *parent) :
    QObject(parent)
{
}

QString AppItem::name() const{
    return m_name;
}

QString AppItem::lversion() const{
    return m_lversion;
}

QString AppItem::rversion() const{
    return m_rversion;
}

QString AppItem::appUrl() const{
    return m_appUrl;
}

QString AppItem::status() const{
    return m_status;
}

QString AppItem::repo() const{
    return m_repo;
}

QString AppItem::description() const{
    return m_description;
}

QString AppItem::deps() const{
    return m_deps;
}

int AppItem::dsize() const{
    return m_dsize;
}

int AppItem::i() const{
    return m_index;
}

void AppItem::setName(const QString &name){
    if(name!=m_name){
        m_name=name;
        emit nameChanged();
    }
}

void AppItem::setLVersion(const QString &lversion){
    if(lversion!=m_lversion){
        m_lversion=lversion;
        emit lversionChanged();
    }
}

void AppItem::setRVersion(const QString &rversion){
    if(rversion!=m_rversion){
        m_rversion=rversion;
        emit rversionChanged();
    }
}

void AppItem::setAppUrl(const QString &appUrl){
    if(appUrl!=m_appUrl){
        m_appUrl=appUrl;
        emit appUrlChanged();
    }
}

void AppItem::setStatus(const QString &status){
    if(status!=m_status){
        m_status=status;
        emit statusChanged();
    }
}

void AppItem::setRepo(const QString &repo){
    if(repo!=m_status){
        m_repo=repo;
        emit repoChanged();
    }
}

void AppItem::setDeps(const QString &deps){
    if(deps!=m_deps){
        m_deps=deps;
        emit depsChanged();
    }
}

void AppItem::setDescription(const QString &description){
    if(description!=m_description){
        m_description=description;
        emit descriptionChanged();
    }
}

void AppItem::setI(const int &i){
    if(i!=m_index){
        m_index=i;
        emit iChanged();
    }
}

void AppItem::setDSize(const int &dsize){
    if(dsize!=m_dsize){
        m_dsize=dsize;
        emit dSizeChanged();
    }
}
