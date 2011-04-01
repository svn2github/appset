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
#ifndef asNIXENGINE_H
#define asNIXENGINE_H

#include "asengine.h"

#include <string>
#include <map>

namespace AS{

    class NIXEngine : public Engine{
    protected:
        typedef std::pair<std::string,std::string> StrPair;
        typedef std::map<std::string,std::string> StrMap;

        StrMap sysinfo;
        StrMap commands;
        StrMap regexps;

        StrMap community;

        std::string pipePath;

        int loadConfigFile(const char *path, StrMap *params, int fErrorRet=1, int paramErrorRet=2);
        virtual int execCmd(std::string command);
        int execQuery(std::list<AS::Package*>* pkgList, unsigned flags=0, AS::Package *package=0, bool remote=false, bool local=false);

        //Community stuff
        bool community_enabled;
        int execComQuery(std::list<AS::Package*>* pkgList, std::string pattern, bool info=false);
    public:
        NIXEngine();
        ~NIXEngine();

        int configure(const char *confFilePath="/etc/appset.conf", const char *pipePath="/tmp/as.tmp",bool force=false);
        int saveConfig(const char *distName, const char *toolName, const char *confWrapperPath="/etc/appset/", const char *confFilePath="/etc/appset.conf");

        virtual int update();
        virtual int upgrade(std::list<Package*>* ignore_packages=0);
        virtual int install(std::list<Package*>* packages, bool local=false);
        virtual int remove(std::list<Package*>* packages);
        virtual std::list<Package*>* queryLocal(unsigned flags, Package *package=0);
        virtual std::list<Package*>* queryRemote(unsigned flags, Package *package=0);
        virtual std::list<Package*>* checkDeps(Package *package, bool install, bool upgrade=false, bool local=false);
        virtual int getProgressSize(Package *package, bool deps=false);

        virtual int removeLock();
        virtual int cacheSize();
        virtual int cleanCache();

        virtual std::string getNewsUrl(std::string lang);
        virtual std::string getLocalExt(){return commands["local_ext"];};

        virtual std::string getTool(){return sysinfo["tool"];};

        //Community stuff
        virtual bool isCommunityEnabled(){return community_enabled;};
        virtual std::list<Package*>* com_search(std::string pattern);
        virtual std::list<Package*>* com_info(std::string pattern);
        virtual int com_install(std::string pattern);
        virtual int com_remove(std::string pattern);
        virtual int com_upgrade(std::string pattern);
        virtual std::string getCommunityName(){return community["community_name"];};
    };

}

#endif // asNIXENGINE_H
