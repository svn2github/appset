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
#ifndef asENGINE_H
#define asENGINE_H

#include "aspackage.h"

#include <list>

namespace AS{

#define as_QUERY_UPGRADABLE  0x0001
#define as_QUERY_BY_NAME     0x0002
#define as_QUERY_ALL_INFO    0x0004
#define as_EXPERT_QUERY      0x0008

#define as_QUERY_DEPS 0x0010
#define as_UPGRADE_DEPS 0x0020
#define as_MERGE_QUERIES 0x0040

    class EngineListener{
    public:
        virtual void step(const char *content) = 0;
    };

    class Engine {
    protected:
        std::list<EngineListener*> listeners;
    public:
        virtual int update() = 0;
        virtual int upgrade(std::list<Package*>* ignore_packages=0) = 0;
        virtual int install(std::list<Package*>* packages) = 0;
        virtual int remove(std::list<Package*>* packages) = 0;
        virtual std::list<Package*>* queryLocal(unsigned flags=0, Package *package=0) = 0;
        virtual std::list<Package*>* queryRemote(unsigned flags=0, Package *package=0) = 0;
        virtual std::list<Package*>* checkDeps(Package *package, bool install, bool upgrade=false) = 0;

        //Requires name and ksize setted
        virtual int getProgressSize(Package *package, bool deps=false) = 0;

        virtual void addListener(EngineListener *listener){listeners.insert(listeners.end(),listener);}
        virtual void removeListener(EngineListener *listener){listeners.remove(listener);}
        virtual void notifyListeners(const char *content){for(std::list<EngineListener*>::iterator it=listeners.begin();it!=listeners.end();it++)(*it)->step(content);}
    };

}

#endif // asENGINE_H
