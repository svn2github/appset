#include "inputprovider.h"

bool InputProvider::evaluate(const QString &content){
    bool ret=false;

    for(int i=0;!ret && i<quests.size();++i){
        ret=content.contains(quests.at(i));
    }

    if(ret) emit userQuery(content);

    return ret;
}

#include <QFile>
#include <QTextStream>
void InputProvider::loadQuests(QString filepath){
    QFile file(filepath);
    QTextStream stream(&file);
    if(file.open(QIODevice::ReadOnly)){
        while(!file.atEnd()){
              quests.append(QRegExp(stream.readLine()));
        }

        file.close();
    }
}
