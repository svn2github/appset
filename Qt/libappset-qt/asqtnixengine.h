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
#ifndef asQTNIXENGINE_H
#define asQTNIXENGINE_H

#include <asnixengine.h>
#include <QString>

namespace AS{

    class QTNIXEngine : public NIXEngine {
    protected:        
        int execCmd(std::string command);
    public:
        QString getConfErrStr(int errno);

        int compareVersions(const QString &s1, const QString &s2);
    };

}

#endif // asQTNIXENGINE_H