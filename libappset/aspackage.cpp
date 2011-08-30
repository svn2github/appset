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
#include "aspackage.h"

#define NO_PKG_INFO_STR "NO INFO"

AS::Package::Package(bool installed){
    name = new std::string(NO_PKG_INFO_STR);
    description = new std::string(NO_PKG_INFO_STR);
    license = new std::string(NO_PKG_INFO_STR);
    url = new std::string(NO_PKG_INFO_STR);
    group = new std::string(NO_PKG_INFO_STR);
    localVersion = new std::string(NO_PKG_INFO_STR);
    remoteVersion = new std::string(NO_PKG_INFO_STR);
    repository = new std::string(NO_PKG_INFO_STR);

    this->installed = installed;

    queried = false;

    ksize = 0;
}

AS::Package::Package(const Package &p){
    copy(p);
}

AS::Package& AS::Package::operator=(const AS::Package &p){
    //ANTIALIASING
    if(this==&p)return *this;

    clean();
    copy(p);

    return *this;
}

void AS::Package::copy(const Package &p){
    name = new std::string(*p.name);
    description = new std::string(*p.description);
    license = new std::string(*p.license);
    url = new std::string(*p.url);
    group = new std::string(*p.group);
    localVersion = new std::string(*p.localVersion);
    remoteVersion= new std::string(*p.remoteVersion);
    repository= new std::string(*p.repository);

    installed = p.installed;

    queried=p.queried;

    ksize=p.ksize;
}

void AS::Package::clean(){
    delete name;
    delete description;
    delete license;
    delete url;
    delete group;
    delete localVersion;
    delete remoteVersion;
    delete repository;
}

using namespace std;
AS::Package::~Package(){
    clean();
}

#undef NO_PKG_INFO_STR
