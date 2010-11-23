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
#ifndef asPACKAGE_H
#define asPACKAGE_H

#include <string>

namespace AS{

    class Package {
    protected:
        std::string name;
        std::string description;
        std::string license;
        std::string url;
        std::string group;
        std::string localVersion;
        std::string remoteVersion;

        int ksize;

        bool installed;

        bool queried;   //Complete info queried
    public:
        Package(bool installed=false);

        //GETTERS
        std::string getName(){return name;}
        std::string getDescription(){return description;}
        std::string getLicense(){return license;}
        std::string getURL(){return url;}
        std::string getGroup(){return group;}
        std::string getLocalVersion(){return localVersion;}
        std::string getRemoteVersion(){return remoteVersion;}
        int getSize(){return ksize;} //In Kbyte
        bool isInstalled(){return installed;}
        bool isQueried(){return queried;}

        //SETTERS
        void setName(std::string name){this->name=name;}
        void setDescription(std::string description){this->description=description;}
        void setLicense(std::string license){this->license=license;}
        void setURL(std::string url){this->url=url;}
        void setGroup(std::string group){this->group=group;}
        void setLocalVersion(std::string version){this->localVersion=version;}
        void setRemoteVersion(std::string version){this->remoteVersion=version;}
        void setInstalled(bool installed=true){this->installed=installed;}
        void setQueried(bool queried=true){this->queried=queried;}
        void setSize(int ks){ksize=ks;}

        void setQueryResult(std::string description=0, std::string url=0, std::string group=0, std::string localVersion=0, std::string remoteVersion=0);

    };

}

#endif // asPACKAGE_H
