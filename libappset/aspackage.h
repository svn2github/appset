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
        std::string *name;
        std::string *description;
        std::string *license;
        std::string *url;
        std::string *group;
        std::string *localVersion;
        std::string *remoteVersion;
        std::string *repository;

        int ksize;

        bool installed;

        bool queried;   //Complete info queried
    public:
        Package(bool installed=false);
        ~Package();

        //GETTERS
        std::string getName(){return *name;}
        std::string getDescription(){return *description;}
        std::string getLicense(){return *license;}
        std::string getURL(){return *url;}
        std::string getGroup(){return *group;}
        std::string getLocalVersion(){return *localVersion;}
        std::string getRemoteVersion(){return *remoteVersion;}
        std::string getRepository(){return *repository;}
        int getSize(){return ksize;} //In Kbyte
        bool isInstalled(){return installed;}
        bool isQueried(){return queried;}

        //SETTERS
        void setName(const char *name){this->name->assign(name);}
        void setDescription(const char *description){this->description->assign(description);}
        void setLicense(const char *license){this->license->assign(license);}
        void setURL(const char *url){this->url->assign(url);}
        void setGroup(const char *group){this->group->assign(group);}
        void setLocalVersion(const char *version){this->localVersion->assign(version);}
        void setRemoteVersion(const char *version){this->remoteVersion->assign(version);}
        void setRepository(const char *repository){this->repository->assign(repository);}
        void setInstalled(bool installed=true){this->installed=installed;}
        void setQueried(bool queried=true){this->queried=queried;}
        void setSize(int ks){ksize=ks;}

    };

}

#endif // asPACKAGE_H
