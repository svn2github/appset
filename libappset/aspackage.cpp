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

using namespace std;
AS::Package::~Package(){
    delete name;
    delete description;
    delete license;
    delete url;
    delete group;
    delete localVersion;
    delete remoteVersion;
    delete repository;
}

#undef NO_PKG_INFO_STR
