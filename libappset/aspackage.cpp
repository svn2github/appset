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
    name = description = license = url = group = localVersion = remoteVersion = repository = std::string(NO_PKG_INFO_STR);

    this->installed = installed;

    queried = false;

    ksize = 0;
}


void AS::Package::setQueryResult(std::string description, std::string url, std::string group, std::string localVersion, std::string remoteVersion, std::string repository){
    setDescription(description);
    setURL(url);
    setGroup(group);
    setLocalVersion(localVersion);
    setRemoteVersion(remoteVersion);
    setRepository(repository);

    queried = true;
}

#undef NO_PKG_INFO_STR
